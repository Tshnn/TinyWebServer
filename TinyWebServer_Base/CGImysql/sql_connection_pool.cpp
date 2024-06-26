#include <mysql/mysql.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <list>
#include <pthread.h>
#include <iostream>
#include "sql_connection_pool.h"
using namespace std;
connection_pool::connection_pool()
{
    m_CurConn = 0;  // 当前已使用的连接数初始化为0
    m_FreeConn = 0; // 当前空闲的连接数初始化为0
}

connection_pool *connection_pool::GetInstance()
{
    static connection_pool connPool;
    return &connPool;
}

//构造初始化
void connection_pool::init(string url, string User, string PassWord, string DBName, int Port, int MaxConn)
{
    m_url = url;
    m_Port = Port;
    m_User = User;
    m_PassWord = PassWord;
    m_DatabaseName = DBName;

    for (int i = 0; i < MaxConn; i++)
    {
        // 创建每个MySQL连接并初始化
        MYSQL *con = NULL;
        con = mysql_init(con);

        if (con == NULL)
        {
            // LOG_ERROR("MySQL Error");
            exit(1);
        }

        // 与MySQL建立连接:给定IP地址，数据库用户名和密码，数据库名
        con = mysql_real_connect(con, url.c_str(), User.c_str(), PassWord.c_str(), DBName.c_str(), Port, NULL, 0);

        if (con == NULL)
        {
            //LOG_ERROR("MySQL Error");
            exit(1);
        }
        connList.push_back(con);
        ++m_FreeConn;
    }

    reserve = sem(m_FreeConn); // 用信号量表示可用连接数

    m_MaxConn = m_FreeConn;
}

//当有请求时，从数据库连接池中返回一个可用连接，更新使用和空闲连接数
MYSQL *connection_pool::GetConnection()
{
    MYSQL *con = NULL;

    if (0 == connList.size())
        return NULL;

    reserve.wait(); // 表示可用连接数的信号量-1

    lock.lock();    // 由于数据库连接池是共享数据，所以访问时加锁

    con = connList.front();
    connList.pop_front();

    --m_FreeConn;
    ++m_CurConn;

    lock.unlock();  // 解锁
    return con;
}

//释放当前使用的连接
bool connection_pool::ReleaseConnection(MYSQL *con)
{
    if (NULL == con)
        return false;

    lock.lock();

    connList.push_back(con);    // 归还mysql连接到连接池
    ++m_FreeConn;
    --m_CurConn;

    lock.unlock();

    reserve.post();             // 表示可用连接数的信号量+1
    return true;
}

//销毁数据库连接池
void connection_pool::DestroyPool()
{

    lock.lock();
    if (connList.size() > 0)
    {
        list<MYSQL *>::iterator it;
        for (it = connList.begin(); it != connList.end(); ++it)
        {
            MYSQL *con = *it;
            mysql_close(con);
        }
        m_CurConn = 0;
        m_FreeConn = 0;
        connList.clear();
    }

    lock.unlock();
}

//当前空闲的连接数
int connection_pool::GetFreeConn()
{
    return this->m_FreeConn;
}

connection_pool::~connection_pool()
{
    DestroyPool();
}

connectionRAII::connectionRAII(MYSQL **SQL, connection_pool *connPool)
{
    *SQL = connPool->GetConnection();
    conRAII = *SQL;
    poolRAII = connPool;
}

connectionRAII::~connectionRAII()
{
    poolRAII->ReleaseConnection(conRAII);
}