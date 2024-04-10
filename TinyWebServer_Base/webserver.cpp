#include "webserver.h"

WebServer::WebServer()
{
    // 每个用户专属的请求对象，保存了各种信息：HTTP信息
    users = new http_conn[MAX_FD];

    //root文件夹路径
    char server_path[200];
    getcwd(server_path, 200);   // :把当前工作目录的绝对地址保存到 buf 中
    char root[6] = "/root";
    m_root = (char *)malloc(strlen(server_path) + strlen(root) + 1);
    strcpy(m_root, server_path);
    strcat(m_root, root);
    cout<<m_root<<endl;
}
WebServer::~WebServer()
{
    close(m_epollfd);
    close(m_listenfd);
    delete[] users;
    delete m_pool;
}

void WebServer::init(int port, string user, string passWord, string databaseName,
                     int trigmode, int sql_num, int thread_num, int actor_model)
{
    m_port = port;
    m_user = user;
    m_passWord = passWord;
    m_databaseName = databaseName;
    m_sql_num = sql_num;
    m_thread_num = thread_num;
    m_TRIGMode = trigmode;
    m_actormodel = actor_model;
}

void WebServer::eventListen()
{
    // 创建socket
    m_listenfd = socket(PF_INET, SOCK_STREAM, 0);

    int optval = 1;
    setsockopt(m_listenfd ,SOL_SOCKET ,SO_REUSEPORT,&optval, sizeof(optval));
    // 绑定socket
    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(m_port);
    ret = bind(m_listenfd, (struct sockaddr *)&address, sizeof(address));

    // 监听
    ret = listen(m_listenfd, 5);

    // epoll创建内核事件表
    m_epollfd = epoll_create(5);
    http_conn::m_epollfd = m_epollfd; // 将epollfd传递给http_conn
    // 为监听socket注册EPOLLIN可读事件
    utils.addfd(m_epollfd, m_listenfd, false, m_LISTENTrigmode);
    http_conn::m_epollfd = m_epollfd;
}

void WebServer::trig_mode()
{
    //LT + LT
    if (0 == m_TRIGMode)
    {
        m_LISTENTrigmode = 0;
        m_CONNTrigmode = 0;
    }
        //LT + ET
    else if (1 == m_TRIGMode)
    {
        m_LISTENTrigmode = 0;
        m_CONNTrigmode = 1;
    }
        //ET + LT
    else if (2 == m_TRIGMode)
    {
        m_LISTENTrigmode = 1;
        m_CONNTrigmode = 0;
    }
        //ET + ET
    else if (3 == m_TRIGMode)
    {
        m_LISTENTrigmode = 1;
        m_CONNTrigmode = 1;
    }
}

void WebServer::eventLoop()
{
    bool timeout = false;
    bool stop_server = false;

    while (!stop_server)
    {
        int number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
        // printf("事件个数 = %d \n",number);
        for (int i = 0; i < number; i++)
        {
            int sockfd = events[i].data.fd;
           // printf("sockfd = %d ; 事件 = %d \n",sockfd,events[i].events);
            //处理新到的客户连接
            if (sockfd == m_listenfd)
            {
                bool flag = dealclinetdata();
                if (false == flag)
                    continue;
                //printf("已接受客户连接\n");
            }
            //处理客户连接上接收到的数据，有可读事件发生
            else if (events[i].events & EPOLLIN)
            {
                // printf("socket = %d 有可读事件 \n",sockfd);
                dealwithread(sockfd);
            } // 有可写事件发生
            else if (events[i].events & EPOLLOUT)
            {
                dealwithwrite(sockfd);
            }
        }
    }
}

bool WebServer::dealclinetdata()
{
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);
    if (0 == m_LISTENTrigmode)  // 监听描述符为LT模式，定时处理
    {
        int connfd = accept(m_listenfd, (struct sockaddr *)&client_address, &client_addrlength);
        users[connfd].init(connfd, client_address, m_root, m_CONNTrigmode,  m_user, m_passWord, m_databaseName);
    }
    else        // 监听描述符为ET模式，一次读完，当然这里只需要接受连接就好。
    {
        while (1)
        {
            int connfd = accept(m_listenfd, (struct sockaddr *)&client_address, &client_addrlength);
            users[connfd].init(connfd, client_address, m_root, m_CONNTrigmode, m_user, m_passWord, m_databaseName);
        }
        return false;
    }
    return true;
}

void WebServer::dealwithread(int sockfd)
{
    //reactor：主线程只负责监听，若有则通知工作线程（将请求放入请求队列中），工作线程负责处理。
    if (1 == m_actormodel)
    {
        //若监测到读事件，将该事件放入请求队列，0表示是可读事件，要读入用户缓冲区中
        m_pool->append(users + sockfd, 0);
        // 定时器相关？
//        while (true)
//        {
//            if (1 == users[sockfd].improv)
//            {
//                if (1 == users[sockfd].timer_flag)
//                {
//                    users[sockfd].timer_flag = 0;
//                }
//                users[sockfd].improv = 0;
//                break;
//            }
//        }
    }
    else
    {
        //proactor
        if (users[sockfd].read_once())
        {
            //若监测到读事件，将该事件放入请求队列
            /*m_pool->append_p(users + sockfd);*/
        }
        else
        {

        }
    }
}

void WebServer::dealwithwrite(int sockfd)
{
    //reactor
    if (1 == m_actormodel)
    {
        m_pool->append(users + sockfd, 1);
    }
//    else
//    {
//        //proactor
//        if (users[sockfd].write())
//        {
//            LOG_INFO("send data to the client(%s)", inet_ntoa(users[sockfd].get_address()->sin_addr));
//
//            if (timer)
//            {
//                adjust_timer(timer);
//            }
//        }
//        else
//        {
//            deal_timer(timer, sockfd);
//        }
//    }
}

void WebServer::thread_pool()
{
    //线程池
    m_pool = new threadpool<http_conn>(m_actormodel, m_connPool, m_thread_num);
}

void WebServer::sql_pool()
{
    //初始化数据库连接池
    m_connPool = connection_pool::GetInstance();
    m_connPool->init("localhost", m_user, m_passWord, m_databaseName, 3306, m_sql_num);

    //初始化数据库读取表
    users->initmysql_result(m_connPool);
}