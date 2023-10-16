#ifndef DB_H
#define DB_H

#include <mysql/mysql.h>
#include <string>

// 数据库操作类
class MySQL
{
public:
    // 初始化数据库连接
    MySQL(std::string ip, unsigned int port, std::string dbname, 
    std::string username, std::string password);
    // 释放数据库连接资源
    ~MySQL();
    // 连接数据库
    bool connect();

    // 查询操作
    MYSQL_RES *query(std::string sql);
    // 更新操作
    bool update(std::string sql);
    // 获取连接
    MYSQL* getConnection();

private:
    MYSQL *m_conn;
    std::string m_ip;
    unsigned int m_port;
    std::string m_dbname;
    std::string m_username;
    std::string m_password;
};

#endif