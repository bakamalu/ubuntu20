#ifndef LOCKER_H
#define LOCKER_H
//若没有定义该变量，则定义

#include<exception>
#include<pthread.h>
#include<semaphore.h>

//封装信号量的类:
class sem
{
private:
    sem_t m_sem;

public:

    sem()
    {
        //信号量初值设置为0，只允许1个进程内部使用的信号量
        if(sem_init(&m_sem,0,0)!=0)
        {
            //构造函数无返回值，可通过抛出异常报告错误
            throw std::exception();
        }
    }

    //析构:销毁信号量
    ~sem()
    {
        sem_destroy(&m_sem);
    }

    //等待信号量
    bool wait()
    {
        //操作前，信号量值为0则阻塞，直到有post将其加1
        return sem_wait(&m_sem)==0;
    }

    //增加信号量
    bool post()
    {
        //信号量加一
        return sem_post(&m_sem)==0;
    }
};

//封装互斥锁的类
class locker
{
private:
    pthread_mutex_t m_mutex;

public:
    //构造:创建、初始化互斥锁
    locker()
    {
        //无指定属性
        if(pthread_mutex_init(&m_mutex,NULL)!=0)
        {
            throw std::exception();
        }        
    }

    //销毁互斥量
    ~locker()
    {
        pthread_mutex_destroy(&m_mutex);
    }

    //获取互斥锁
    bool lock()
    {
        return pthread_mutex_lock(&m_mutex)==0;
        //获取成功返回true
    }

    //释放互斥锁
    bool unlock()
    {
        return pthread_mutex_unlock(&m_mutex)==0;
        //释放成功返回true
    }
};

//封装条件变量的类
class cond
{
private:
    pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;

public:
    //构造:创建并初始化条件变量和锁
    cond()
    {
        //锁初始化
        if(pthread_mutex_init(&m_mutex,NULL)!=0)
        {
            throw std::exception();
        }
        if(pthread_cond_init(&m_cond,NULL)!=0)
        {
            //此处出问题，先将上一句建立的锁耗费的资源给释放
            pthread_mutex_destroy(&m_mutex);
            throw std::exception();
        }
    }

    //析构:销毁条件变量和锁
    ~cond()
    {
        pthread_mutex_destroy(&m_mutex);
        pthread_cond_destroy(&m_cond);
    }

    //等待条件变量
    bool wait()
    {
    int ret=0;
    //调用pthread_cond_wait前必须保证互斥锁加锁
    pthread_mutex_lock(&m_mutex);

    //临界区
    //阻塞该线程，直到收到信号m_cond
    ret=pthread_cond_wait(&m_cond,&m_mutex);
    //临界区

    pthread_mutex_unlock(&m_mutex);
    return ret==0;//信号量
    }

    //唤醒一个等待条件变量的线程
    bool signal()
    {
        return pthread_cond_signal(&m_cond)==0;
        //唤醒成功返回true
    }

    //唤醒所有等待该条件变量的线程
    bool signal_all()
    {
        return pthread_cond_broadcast(&m_cond)==0;
    }
};
#endif
