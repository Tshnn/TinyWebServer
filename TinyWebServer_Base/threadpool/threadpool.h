#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"

// 线程池中的线程要从请求队列中拿出请求并处理

/* 线程池m_threads、请求队列list、Http请求 */
template <typename T>
class threadpool
{
public:
    /* -- 线程池 -- */
    threadpool(int actor_model,connection_pool *connPool,int thread_number = 8,int max_request = 10000); /*thread_number是线程池中线程的数量，max_requests是请求队列中最多允许的、等待处理的请求的数量*/
    ~threadpool();

    /* -- 将请求加入到请求队列中 -- */
    /* 由于每个线程都要从请求队列中取请求，所以请求队列属于共享数据，需要使用线程同步 */
    bool append(T *request,int state);
    bool append_p(T *request);

private:
    /* 工作线程，从工作队列中取任务并执行*/
    static void *worker(void *arg);
    void run();

private:
    pthread_t *m_threads;       //  线程池，大小是m_thread_number
    int m_thread_number;        //  线程池中的线程数

    std::list<T *> m_workqueue; //  请求队列
    int m_max_requests;          //  请求队列中允许的最大请求数
    locker m_queuelocker;       //  保护请求队列的互斥锁
    sem m_queuestat;            //  表示任务数量的信号量

    int m_actor_model;          // 事件处理模式：reactor，Proactor

    connection_pool *m_connPool;  //连接池

};

template <typename T>
threadpool<T>::threadpool(int actor_model, connection_pool *connPool, int thread_number, int max_requests): m_actor_model(actor_model),m_thread_number(thread_number), m_max_requests(max_requests), m_threads(NULL),m_connPool(connPool)
{
    m_threads = new pthread_t[m_thread_number];
    for(int i=0;i<thread_number;i++)
    {
        pthread_create(m_threads+i,NULL,worker,this);
        pthread_detach(m_threads[i]);
    }
}
template <typename T>
threadpool<T>::~threadpool()
{
    delete[] m_threads;
}
template <typename T>
void* threadpool<T>::worker(void *arg)
{
    threadpool *pool = (threadpool *)arg;
    pool->run();
    return pool;
}
template <typename T>
void threadpool<T>::run()
{
    while(true)
    {
        // 从工作队列中取出请求
        m_queuestat.wait(); // 表示请求队列中任务数量-1的信号量
        // printf("工作线程处理 \n");
        m_queuelocker.lock();
        if (m_workqueue.empty())
        {
            m_queuelocker.unlock();
            continue;
        }
        T *request = m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelocker.unlock();

        // Reactor模式：主线程负责监听事件发生，事件处理交由工作线程处理
        if(m_actor_model == 1)
        {
            if(request->m_state == 0) // 若是读请求,工作线程负责将TCP内核读缓冲区的数据拷贝到用户缓冲区中
            {
                // printf("读请求 \n");
                if(request->read_once())
                {
                    //printf("读完了 \n");
                    // 表示已经把http请求读到用户缓冲区m_read_buf中了
                    request->improv = 1; // 暂时未知，可能和定时器有关
                    // Reactor模式工作线程开始处理请求
                    connectionRAII mysqlcon(&request->mysql, m_connPool); // 从连接池取一个连接出来给这个请求，因为要访问数据库了
                    request->process();
                }
            }
            else                     // 若是写请求
            {
                if (request->write())
                {
                    request->improv = 1;
                }
                else
                {
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            }
        }
    }
}

template <typename T>
bool threadpool<T>::append(T *request, int state)
{
    // 添加请求到请求队列中,表示请求数量的信号量+1
    m_queuelocker.lock();
    if (m_workqueue.size() >= m_max_requests)
    {
        m_queuelocker.unlock();
        return false;
    }
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post();
    request->m_state = state;  // 0表示是可读事件
    return true;
}

#endif
