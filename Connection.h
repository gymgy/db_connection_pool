#pragma once

#include <mysql/mysql.h>
#include <string>
#include <ctime>

using std::string;

class Connection
{
public:
    Connection();
    ~Connection();

    // 连接数据库
    bool connect(string ip, unsigned short port, string user, string passwd, string dbname);

    // 更新操作
    bool update(string sql);

    // 查询操作
    MYSQL_RES *query(string sql);

    // 刷新连接的起始空闲时间
    void refreshAliveTime() { _alivetime = clock(); }

    // 获取连接的起始空闲时间
    clock_t getAliveTime() const { return _alivetime; }

private:
    MYSQL *_conn;       // 一条连接
    clock_t _alivetime; // 记录本条连接进入空闲状态的起始时间
};