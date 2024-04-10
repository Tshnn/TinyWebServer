// 数据库连接池
#ifndef _CONNECTION_POOL_H
#define _CONNECTION_POOL_H

#include <stdio.h>
#include <list>
#include <mysql/mysql.h>
#include <error.h>
#include <string.h>
#include <iostream>
#include <string>
#include "../lock/locker.h"

using namespace std;
class connection_pool
{
public:
    MYSQL *GetConnection();     // 获取MySQL数据库连接
    bool ReleaseConnection(MYSQL *conn);   //释放连接
    int GetFreeConn();                  // 获取连接
    void DestroyPool();                 // 销毁所有连接

    static connection_pool *GetInstance();  // 单例模式

    void init(string url,string User,string PassWord,string DataBaseName,int Port,int MaxConn);

private:
    connection_pool();
    ~connection_pool();

    int m_MaxConn;  //最大连接数
    int m_CurConn;  //当前已使用的连接数
    int m_FreeConn; //当前空闲的连接数
    locker lock;    // 互斥锁
    list<MYSQL *> connList; //连接池,通过list链表保存着多个与MySQL数据库的连接
    sem reserve;    // 信号量

public:
    string m_url;			 //主机地址
    string m_Port;		 //数据库端口号
    string m_User;		 //登陆数据库用户名
    string m_PassWord;	 //登陆数据库密码
    string m_DatabaseName; //使用数据库名
    int m_close_log;	//日志开关
};


/*
    数据库连接池附属类，资源申请即初始化，构造函数会创建一个连接池，并从中获取一个可用连接，
    析构函数会释放这个连接。
 */

class connectionRAII{

public:
    // con是传出参数，返回的是mysql连接对象
    connectionRAII(MYSQL **con, connection_pool *connPool);
    ~connectionRAII();

private:
    MYSQL *conRAII;
    connection_pool *poolRAII;
};
#endif
