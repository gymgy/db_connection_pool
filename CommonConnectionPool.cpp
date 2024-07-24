#include "CommonConnectionPool.h"
#include "public.h"

#include <functional>
#include <iostream>

ConnectionPool *ConnectionPool::instance()
{
    static ConnectionPool connectionPool;
    return &connectionPool;
}

shared_ptr<Connection> ConnectionPool::getConnection()
{
    unique_lock<mutex> lock(_queueMutex);
    while (_connectionQue.empty()) {
        if (cv_status::timeout == cv.wait_for(lock, chrono::milliseconds(_connectionTimeout))) {
            // 如果等待了超过_connectionTimeout时间，生产者进程还没有创建完新连接
            if (_connectionQue.empty()) {
                LOG("获取空闲连接超时！");
                return nullptr;
            }
        }
    }
    // 自定义shared_ptr的析构方式，将connection归还给queue
    shared_ptr<Connection> sp(_connectionQue.front(),
                              [&](Connection *pcon) {
                                  unique_lock<mutex> lock(_queueMutex);
                                  pcon->refreshAliveTime();
                                  _connectionQue.push(pcon);
                              });
    _connectionQue.pop();
    cv.notify_all(); // 消费完连接后，通知生产者线程检查，如果队列空了，生产连接

    return sp;
}

ConnectionPool::ConnectionPool()
{
    if (!loadConfigFile())
        return;

    // 创建初始数量的连接
    for (int i = 0; i < _initSize; ++i) {
        Connection *p = new Connection();
        p->connect(_ip, _port, _username, _password, _dbname);
        p->refreshAliveTime(); // 刷新开始空闲的时间点
        _connectionQue.push(p);
        _connectionCnt++;
    }

    // 启动一个新的线程，作为连接的生产者
    thread produce(std::bind(&ConnectionPool::produceConnectionTask, this));
    produce.detach();

    // 启动一个新的定时线程，扫描超过_maxIdleTime事件的空闲连接，进行连接回收
    thread scanner(std::bind(&ConnectionPool::scannerConnectionTask, this));
    scanner.detach();
}

bool ConnectionPool::loadConfigFile()
{
    FILE *pf = fopen("mysql.ini", "r");
    if (pf == nullptr) {
        LOG("mysql.ini file is not exist!");
        return false;
    }

    while (!feof(pf)) {
        char line[1024] = "";
        fgets(line, 1024, pf);
        string str = line;
        int idx = str.find('=', 0);
        if (idx == -1) { // 无效配置
            continue;
        }

        int end_idx = str.find('\n', idx);
        string key = str.substr(0, idx);
        string value = str.substr(idx + 1, end_idx - idx - 1);
        if (key == "ip") {
            _ip = value;
        } else if (key == "port") {
            _port = atoi(value.c_str());
        } else if (key == "username") {
            _username = value;
        } else if (key == "password") {
            _password = value;
        } else if (key == "dbname") {
            _dbname = value;
        } else if (key == "initSize") {
            _initSize = atoi(value.c_str());
        } else if (key == "maxSize") {
            _maxSize = atoi(value.c_str());
        } else if (key == "maxIdleTime") {
            _maxIdleTime = atoi(value.c_str());
        } else if (key == "connectionTimeOut") {
            _connectionTimeout = atoi(value.c_str());
        }
    }

    return true;
}

// 运行在独立的线程中，生产新的连接
void ConnectionPool::produceConnectionTask()
{
    for (;;) {
        unique_lock<mutex> lock(_queueMutex);
        while (!_connectionQue.empty()) {
            cv.wait(lock); // 队列不空，进入等待状态
        }
        // 连接数没有达到上限，继续创建新连接
        if (_connectionCnt < _maxSize) {
            Connection *p = new Connection();
            p->connect(_ip, _port, _username, _password, _dbname);
            p->refreshAliveTime();
            _connectionQue.push(p);
            _connectionCnt++;
        }
        // 通知消费者线程，可以消费连接了
        cv.notify_all();
    }
}

void ConnectionPool::scannerConnectionTask()
{
    for (;;) {
        // 通过sleep模拟定时效果 定时扫描
        this_thread::sleep_for(chrono::seconds(_maxIdleTime));

        // 扫描整个队列，释放多余连接
        unique_lock<mutex> lock(_queueMutex);
        while (_connectionCnt > _initSize) {
            Connection *p = _connectionQue.front();
            if (p->getAliveTime() >= (_maxIdleTime * 1000)) { //_maxIdleTime单位是毫秒
                _connectionQue.pop();
                _connectionCnt--;
                delete p; // 调用~Connection()释放连接
            } else {
                break; // 队头的连接没有超过_maxIdleTime，其它连接肯定没有
            }
        }
    }
}
