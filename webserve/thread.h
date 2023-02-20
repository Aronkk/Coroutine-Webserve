#ifndef __SYLAR_THREAD_H__
#define __SYLAR_THREAD_H__

#include <thread>
#include <functional>
#include <memory>
#include <string>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <atomic>
#include "noncopyable.h"

namespace sylar{

class Semaphore : Noncopyable{
public:
    Semaphore(uint32_t count = 0);
    ~Semaphore();

    void wait();
    void notify();
// private:
//     Semaphore(const Semaphore&) = delete;
//     Semaphore(const Semaphore&&) = delete;
//     Semaphore operator=(const Semaphore&) = delete;
private:
    sem_t m_semaphore;
};

// 局部模板锁，构造函数加锁，析构函数解锁
template<class T>
struct ScopedLockImpl {
public:
    // 构造函数，上锁
    ScopedLockImpl(T& mutex)
        :m_mutex(mutex) {
        m_mutex.lock();
        m_locked = true;
    }

    // 析构函数,自动释放锁
    ~ScopedLockImpl() {
        unlock();
    }

    // 加锁
    void lock() {
        if(!m_locked) {
            m_mutex.lock();
            m_locked = true;
        }
    }

    // 解锁
    void unlock() {
        if(m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:   
    T& m_mutex;     // mutex
    bool m_locked;  // 判断是否已上锁
};

//局部读锁模板实现
template<class T>
struct ReadScopedLockImpl {
public:
    ReadScopedLockImpl(T& mutex)
        :m_mutex(mutex) {
        m_mutex.rdlock();
        m_locked = true;
    }

    ~ReadScopedLockImpl() {
        unlock();
    }

    void lock() {
        if(!m_locked) {
            m_mutex.rdlock();
            m_locked = true;
        }
    }

    void unlock() {
        if(m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    T& m_mutex;
    bool m_locked;
};

// 局部写锁模板实现
template<class T>
struct WriteScopedLockImpl {
public:
    WriteScopedLockImpl(T& mutex)
        :m_mutex(mutex) {
        m_mutex.wrlock();
        m_locked = true;
    }

    ~WriteScopedLockImpl() {
        unlock();
    }

    void lock() {
        if(!m_locked) {
            m_mutex.wrlock();
            m_locked = true;
        }
    }

    void unlock() {
        if(m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    T& m_mutex;
    bool m_locked;
};

// 互斥量
class Mutex : Noncopyable {
public: 
    // 局部锁
    typedef ScopedLockImpl<Mutex> Lock;

    Mutex() {
        pthread_mutex_init(&m_mutex, nullptr);
    }

    ~Mutex() {
        pthread_mutex_destroy(&m_mutex);
    }

    // 加锁
    void lock() {
        pthread_mutex_lock(&m_mutex);
    }

    // 解锁
    void unlock() {
        pthread_mutex_unlock(&m_mutex);
    }
private:
    pthread_mutex_t m_mutex;    // mutex
};

// 空锁(用于调试)，空锁比不加锁的写入速度要快将近20倍，需要进行优化
class NullMutex : Noncopyable {
public:
    typedef ScopedLockImpl<NullMutex> Lock;
    NullMutex() {}
    ~NullMutex() {}
    void lock() {}
    void unlock() {}
};

// 读写互斥量
class RWMutex : Noncopyable {
public:

    typedef ReadScopedLockImpl<RWMutex> ReadLock;       // 局部读锁
    typedef WriteScopedLockImpl<RWMutex> WriteLock;     // 局部写锁

    RWMutex() {
        pthread_rwlock_init(&m_lock, nullptr);
    }

    ~RWMutex() {
        pthread_rwlock_destroy(&m_lock);
    }

    // 上读锁
    void rdlock() {
        pthread_rwlock_rdlock(&m_lock);
    }

    // 上写锁
    void wrlock() {
        pthread_rwlock_wrlock(&m_lock);
    }

    // 解锁
    void unlock() {
        pthread_rwlock_unlock(&m_lock);
    }
private:
    pthread_rwlock_t m_lock;
};

// 空读写锁(用于调试)
class NullRWMutex : Noncopyable {
public:
    typedef ReadScopedLockImpl<NullMutex> ReadLock;
    typedef WriteScopedLockImpl<NullMutex> WriteLock;
    NullRWMutex() {}
    ~NullRWMutex() {}
    void rdlock() {}
    void wrlock() {}
    void unlock() {}
};

class Thread {
public:
    typedef std::shared_ptr<Thread> ptr;
    Thread(std::function<void()> cb, const std::string& name);
    ~Thread();

    pid_t getId() const { return m_id;}
    const std::string& getName() const { return m_name;}

    void join();                // 等待线程执行完成
    static Thread* GetThis();   // 获取当前的线程指针

    static const std::string& GetName();            //获取当前的线程名称
    static void SetName(const std::string& name);   // 设置当前线程名称
private:   
    // 禁止 thread 的拷贝
    // 因为互斥量和互斥信号量都不能进行拷贝，不然会破坏其作用
    Thread(const Thread&) = delete;
    Thread(const Thread&&) = delete;
    Thread operator=(const Thread&) = delete;

    static void* run(void* arg);    // 线程执行函数
private:
    pid_t m_id = -1;                // 线程id
    pthread_t m_thread = 0;         // 线程结构
    std::function<void()> m_cb;     // 线程执行函数
    std::string m_name;             // 线程名称    
    Semaphore m_semaphore;          // 信号量
};

// 自旋锁 -- 只有非常清楚自己在干什么，并且锁阻塞时间能够准确估计的时候才使用
class Spinlock : Noncopyable{
public:
    typedef ScopedLockImpl<Spinlock> Lock;

    Spinlock() {
        pthread_spin_init(&m_mutex, 0);
    }

    ~Spinlock() {
        pthread_spin_destroy(&m_mutex);
    }

    void lock() {
        pthread_spin_lock(&m_mutex);
    }

    void unlock() {
        pthread_spin_unlock(&m_mutex);
    }
private:

    pthread_spinlock_t m_mutex;    // 自旋锁
};

// 原子锁
class CASLock : Noncopyable{
public:
    typedef ScopedLockImpl<CASLock> Lock;

    CASLock() {
        m_mutex.clear();
    }

    ~CASLock() {
    }

    void lock() {
        while(std::atomic_flag_test_and_set_explicit(&m_mutex, std::memory_order_acquire));
    }

    void unlock() {
        std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
    }
private:
    volatile std::atomic_flag m_mutex;      // 原子状态
};

}

#endif