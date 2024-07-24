#include "Connection.h"
#include "public.h"

#include <iostream>

using namespace std;

Connection::Connection()
{
    // 初始化数据库连接
    _conn = mysql_init(nullptr);
}

Connection::~Connection()
{
    // 释放数据库连接资源
    if (_conn != nullptr) {
        mysql_close(_conn);
    }
}

bool Connection::connect(string ip, unsigned short port, string user, string passwd, string dbname)
{
    MYSQL *p = mysql_real_connect(_conn, ip.c_str(), user.c_str(), passwd.c_str(), dbname.c_str(), port, nullptr, 0);
    return p != nullptr;
}

bool Connection::update(string sql)
{
    if (mysql_query(_conn, sql.c_str())) {
        LOG("更新失败" + sql);
        cout << mysql_error(_conn) << endl;
        return false;
    }
    return true;
}

MYSQL_RES *Connection::query(string sql)
{
    if (mysql_query(_conn, sql.c_str())) {
        LOG("查询失败" + sql);
        return nullptr;
    }
    return mysql_use_result(_conn);
}
