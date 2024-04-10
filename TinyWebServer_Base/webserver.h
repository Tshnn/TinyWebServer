#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>

#include "./threadpool/threadpool.h"
#include "./http/http_conn.h"
#include "./Utils/Utils.h"

const int MAX_FD = 65536;           //最大文件描述符-用户数-连接数
const int MAX_EVENT_NUMBER = 10000; //最大事件数
const int TIMESLOT = 5;             //最小超时单位

class WebServer
{
public:
    WebServer();
    ~WebServer();

    void init(int port , string user, string passWord, string databaseName,
              int trigmode, int sql_num,int thread_num, int actor_model);

    void thread_pool();
    void sql_pool();
    void log_write();
    void trig_mode();

    void eventListen();
    void eventLoop();
    bool dealclinetdata();
    void dealwithread(int sockfd);
    void dealwithwrite(int sockfd);

public:
    //基础
    int m_port;
    char *m_root;   // 网页资源根路径

    int m_actormodel;  // 1是reactor，0是proactor
    int m_epollfd;
    http_conn *users;   // 每个用户里面都保存着对应的连接socket，用户的读写缓冲区。

    //数据库相关
    connection_pool *m_connPool;
    string m_user;         //登陆数据库用户名
    string m_passWord;     //登陆数据库密码
    string m_databaseName; //使用数据库名
    int m_sql_num;

    //线程池相关
    threadpool<http_conn> *m_pool;
    int m_thread_num;

    //epoll_event相关
    epoll_event events[MAX_EVENT_NUMBER];

    int m_listenfd;
    int m_TRIGMode;        // 监听socket与连接socket的综合工作模式
    int m_LISTENTrigmode;  // 对于监听socket epoll的工作模式是LT(0)还是ET(1)
    int m_CONNTrigmode;    // 对于连接socket epoll的工作模式的是LT(0)还是ET(1)

    // 工具类
    Utils utils;
};
#endif