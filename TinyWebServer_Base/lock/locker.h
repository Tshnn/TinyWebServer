#ifndef WORKINGSPACE_LOCKER_H
#define WORKINGSPACE_LOCKER_H



#include <pthread.h>
#include <semaphore.h>

// 互斥锁 ：加锁pthread_mutex_lock  -> 访问共享数据  -> 解锁pthread_mutex_unlock
class locker
{
public:
    locker()
    {
        pthread_mutex_init(&m_mutex,NULL);
    }
    ~locker()
    {
        pthread_mutex_destroy(&m_mutex);
    }

    bool lock()
    {
        return pthread_mutex_lock(&m_mutex) == 0;
    }
    bool unlock()
    {
        return pthread_mutex_unlock(&m_mutex) == 0;
    }

    pthread_mutex_t *get()
    {
        return &m_mutex;
    }
private:
    pthread_mutex_t m_mutex;
};

// 条件变量：与互斥锁搭配使用，允许线程以无竞争的方式等待特定的条件发生 = 不仅保证了线程安全，还能等待特定条件
// 阻塞线程pthread_cond_wait -> 等待条件满足 -> 唤醒线程pthread_cond_signal
class cond
{
public:
    cond()
    {
        pthread_cond_init(&m_cond,NULL);
    }
    ~cond()
    {
        pthread_cond_destroy(&m_cond);
    }

    bool wait(pthread_mutex_t *m_mutex)
    {
        return pthread_cond_wait(&m_cond,m_mutex) == 0;
    }
    bool signal()
    {
        return pthread_cond_signal(&m_cond) == 0;
    }
    bool broadcat()
    {
        return pthread_cond_broadcast(&m_cond);
    }
private:
    pthread_cond_t m_cond;
};

// 信号量 ： 作用与条件变量作用一样，即达到特定条件，阻塞/解除阻塞线程
// 信号量-1 sem_wait（值为0就阻塞） -> 信号量 + 1 sem_post（唤醒线程）
class   sem
{
public:
    sem()
    {
        sem_init(&m_sem,0,0);
    }
    sem(int num)
    {
        sem_init(&m_sem,0,num);
    }
    ~sem()
    {
        sem_destroy(&m_sem);
    }

    bool wait() // 信号量 - 1
    {
        return sem_wait(&m_sem) == 0;
    }
    bool post() // 信号量 + 1
    {
        return sem_post(&m_sem) == 0;
    }

    int getvalue()
    {
        int num = 0;
        sem_getvalue(&m_sem,&num);
        return num;
    }
private:
    sem_t m_sem;
};
#endif //WORKINGSPACE_LOCKER_H