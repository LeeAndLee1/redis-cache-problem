#include "db.h"
#include <stdio.h>

// 初始化数据库连接
MySQL::MySQL(std::string ip, unsigned int port, std::string dbname, 
    std::string username, std::string password)
{
    m_ip = ip;
    m_port = port;
    m_dbname = dbname;
    m_username = username;
    m_password = password;
    m_conn = mysql_init(nullptr);
    if (m_conn == nullptr)
    {
        printf("init mysql fail!\n");
    }
}

// 释放数据库连接资源
MySQL::~MySQL()
{
    if (m_conn != nullptr)
    {
        mysql_close(m_conn);
    }
}

// 连接数据库
bool MySQL::connect()
{
    MYSQL *p = mysql_real_connect(m_conn, m_ip.c_str(), m_username.c_str(),
                                  m_password.c_str(), m_dbname.c_str(), m_port, nullptr, 0);
    
    if (p != nullptr)
    {
        // C和C++代码默认的编码字符是ASCII，如果不设置，从MySQL上拉下来的中文显示？
        mysql_query(m_conn, "set names gbk");
    }
    else
    {
        printf("connect mysql fail!\n");
    }

    return p;
}

// 查询操作
MYSQL_RES *MySQL::query(std::string sql)
{
    if (mysql_query(m_conn, sql.c_str()))
    {
        printf("select fail!\n");
        return nullptr;
    }
    
    return mysql_use_result(m_conn);
}

// 更新操作
bool MySQL::update(std::string sql)
{
    if (mysql_query(m_conn, sql.c_str()))
    {
        printf("update db fail.\n");
        return false;
    }
    return true;
}

// 获取连接
MYSQL* MySQL::getConnection()
{
    return m_conn;
}