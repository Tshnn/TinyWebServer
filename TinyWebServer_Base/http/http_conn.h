/*  响应行、响应头部、空行、响应体=html网页 */
#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <map>

#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"
#include "../Utils/Utils.h"

class http_conn
{
public:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;
    /* HTTP协议相关：请求方式 与  响应行的状态码描述*/
    enum METHOD
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };


    /* 主状态机的状态：正在检查请求行,请求头部，请求体 */
    enum CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    /* 从状态机分析每行的结果（从状态机状态） */
    enum LINE_STATUS
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };
    /* 处理HTTP请求的结果 */
    enum HTTP_CODE
    {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };
private:
    /* 连接socket相关 */
    int m_sockfd;           // 连接socket
    sockaddr_in m_address;  // 连接socket的地址

    /* 用户读写缓冲区 */
    char m_read_buf[READ_BUFFER_SIZE];
    int m_read_idx;
    char m_write_buf[WRITE_BUFFER_SIZE];
    int m_write_idx;

    /* 状态机相关 */
    int m_checked_idx;
    int m_start_line;
    CHECK_STATE m_check_state;

    /* HTTP协议相关 */
    METHOD m_method;
    char m_real_file[FILENAME_LEN]; // 欲访问的网页资源文件路径
    char *m_url;
    char *m_version;
    char *m_host;
    int m_content_length;
    bool m_linger;  // 是否保持连接
    char *m_file_address;   // 网页资源经过内存映射后的地址
    struct stat m_file_stat;
    struct iovec m_iv[2];
    int m_iv_count;
    int cgi;        //是否启用的POST:GET请求为0，POST请求为1(表示用户提交了数据，服务器要创建相应资源)
    char *m_string; //存储请求体数据(user=barry&password=123)
    int bytes_to_send;      // 用户缓冲区中剩余需要发送的字节数
    int bytes_have_send;   //  已经发送给用户的字节数
    char *doc_root;

    int m_TRIGMode; // 监听socket和连接socket的触发组合模式，EPOLL的两种工作模式：ET+LT
    map<string, string> m_users;

    /* sql相关 */
    char sql_user[100];
    char sql_passwd[100];
    char sql_name[100];

public:
    http_conn() {}
    ~http_conn() {}
    // 接受连接后得到了连接socket，需要初始化与用户相关的信息：用户需要访问的资源路径，用户对应的数据库连接
    void init(int sockfd, const sockaddr_in &addr, char *, int,string user, string passwd, string sqlname);
    void close_conn(bool real_close = true);

    bool read_once();   // 将TCP内核接收缓冲区中的数据读入用户缓冲区m_read_buf中
    void process();     // 处理请求
    bool write();
    sockaddr_in *get_address()
    {
        return &m_address;
    }
    void initmysql_result(connection_pool *connPool);
    int timer_flag;
    int improv; //

private:
    void init();
    HTTP_CODE process_read();  // 处理读的过程：有限状态机解析HTTP请求
    bool process_write(HTTP_CODE ret);

    /* 读请求中相关：有限状态机 */
    LINE_STATUS parse_line();   // 解析一行
    char *get_line() { return m_read_buf + m_start_line; };
    HTTP_CODE parse_request_line(char *text);      // 分析请求行内容
    HTTP_CODE parse_headers(char *text);           // 分析请求头内容
    HTTP_CODE parse_content(char *text);           // 分析请求体内容
    HTTP_CODE do_request();                        //

    /* process_write响应相关*/
    bool add_response(const char *format, ...);
    bool add_content(const char *content);
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_length);
    bool add_content_type();
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();

    void unmap();


public:
    static int m_epollfd;
    static int m_user_count;    // 静态成员，记录用户数量
    MYSQL *mysql;
    // epoll工具类
    Utils utils;
    int m_state;  //读为0, 写为1
};


#endif //HTTP_CONN_H
