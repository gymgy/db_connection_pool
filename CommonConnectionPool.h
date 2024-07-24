#pragma once

#include "Connection.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <memory>
#include <thread>

using namespace std;

class ConnectionPool
{
public:
    // 获取连接池对象实例
    static ConnectionPool *instance();

    // 对外开放的接口，用于从连接池获取可用的空闲连接
    shared_ptr<Connection> getConnection();

private:
    // 单例模式 构造函数私有化
    ConnectionPool();

    // 从配置文件加载配置项
    bool loadConfigFile();

    // 生产新的连接
    void produceConnectionTask();

    // 扫描超过_maxIdelTime的空闲连接，进行对于连接的回收
    void scannerConnectionTask();

    string _ip;
    unsigned short _port;
    string _username;
    string _password;
    string _dbname;
    int _initSize;          // 连接池初始量
    int _maxSize;           // 连接池最大连接量
    int _maxIdleTime;       // 连接池最大空闲时间
    int _connectionTimeout; // 连接池获取连接最大等待时间

    queue<Connection *> _connectionQue; // 存储连接的队列
    mutex _queueMutex;                  // 维护连接队列的线程安全
    atomic_int _connectionCnt;          // 连接池所创建连接的总量
    condition_variable cv;              // 用于连接生产者-消费者线程通信的条件变量（信号量）
};