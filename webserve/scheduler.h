#ifndef __SYLAR_SCHEDULER_H__
#define __SYLAR_SCHEDULER_H__

#include <memory>
#include <vector>
#include <list>
#include <iostream>
#include "fiber.h"
#include "thread.h"

namespace sylar {

// 协程调度器
// 调度器内部维护一个任务队列（list）和一个调度线程池（vector）。
// 开始调度后，线程池从任务队列里按顺序取任务执行，调度线程可以包含caller线程。
// 当全部任务都执行完了，线程池停止调度，等新的任务进来。
// 添加新任务后，通知线程池有新的任务进来了，线程池重新开始运行调度。停止调度时，各调度线程退出，调度器停止工作。

class Scheduler {
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;    // 线程池必须的 互斥锁

    /**
     * @brief 构造函数
     * @param[in] threads 线程数量，默认构造一个线程
     * @param[in] use_caller 是否使用当前调用线程，true 说明该线程可以被调度
     * @param[in] name 协程调度器名称
     */
    Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = "");

    // 虚析构函数，因为这只是一个虚拟的基类，具体的实现要根据子类来判断
    virtual ~Scheduler();

    // 返回协程调度器名称
    const std::string& getName() const { return m_name;}

    // 返回当前协程调度器
    static Scheduler* GetThis();

    // 返回当前协程调度器的调度协程（主协程）
    // 因为scheduler也有线程负责scheduler的任务，故线程也有一个主协程
    static Fiber* GetMainFiber();

    //启动协程调度器
    void start();

    // 停止协程调度器
    void stop();

    // 调度协程 -- （fc 协程或函数，thread 协程执行的线程id,-1标识任意线程)
    // 为什么这里要上锁？上锁完在哪里解锁？
    template<class FiberOrCb>
    void schedule(FiberOrCb fc, int thread = -1) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            // 调用 scheduleNoLock 生成任务队列
            need_tickle = scheduleNoLock(fc, thread);
        }

        // 如果need_tickle为true（调度队列为空）
        if(need_tickle) {
            tickle();
        }
    }

    // 批量调度协程 -- （begin 协程数组的开始，end 协程数组的结束）
    template<class InputIterator>
    void schedule(InputIterator begin, InputIterator end) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            while(begin != end) {
                need_tickle = scheduleNoLock(&*begin, -1) || need_tickle;
                ++begin;
            }
        }
        if(need_tickle) {
            tickle();
        }
    }

    void switchTo(int thread = -1);
    std::ostream& dump(std::ostream& os);
protected:
    // 通知协程调度器有任务了，类似一个信号量
    virtual void tickle();

    // 协程调度函数
    void run();

    // 返回是否可以停止
    virtual bool stopping();

    // 协程无任务可调度时执行idle协程
    virtual void idle();

    // 设置当前的协程调度器
    void setThis();

    // 是否有空闲线程
    bool hasIdleThreads() { return m_idleThreadCount > 0;}

private:
    // 协程调度启动(无锁)，将可用的协程放到任务队列中去，返回值bool类型相当于消息提醒
    template<class FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc, int thread) {
        // need_tickle 提醒是否有可调度的协程
        bool need_tickle = m_fibers.empty();
        FiberAndThread ft(fc, thread);
        if(ft.fiber || ft.cb) {
            m_fibers.push_back(ft);
        }
        // std::cout<<"消息队列的大小："<<m_fibers.size()<<std::endl;
        return need_tickle;
    }
private:
    // 调度器可以执行的对象，协程/函数/线程组，struct默认都是public
    // 这里相当于初始化或者说包装
    struct FiberAndThread {      
        Fiber::ptr fiber;           // 协程    
        std::function<void()> cb;   // 协程执行函数
        int thread;                 // 线程id，用于后续指定线程执行

        // 下面根据传入FiberAndThread的参数不同，重载了不同的构造函数

        // 构造函数 -- （f 协程，thr 线程id，-1表示不指定线程）
        // 第一个直接传递智能指针对象，该对象肯定实在栈上，栈上的局部变量只要执行到花括号就会释放
        // 所以第一个不需要swap，局部变量会自动析构，引用计数自动-1
        FiberAndThread(Fiber::ptr f, int thr)
            :fiber(f), thread(thr) {
        }

        // 构造函数 -- （f 协程指针，thr 线程id）*f = nullptr
        // 但是第二个传递的是智能指针对象的指针，智能指针对象本身在堆区，不会自动调用析构
        // 也就是说只要外部没有由程序员主动析构，引用计数至少为1，所以要swap掉
        FiberAndThread(Fiber::ptr* f, int thr)
            :thread(thr) {
            fiber.swap(*f);
        }

        // 构造函数 -- （f 协程执行函数，thr 线程id)
        FiberAndThread(std::function<void()> f, int thr)
            :cb(f), thread(thr) {
        }

        // 构造函数 -- (f 协程执行函数指针, thr 线程id)
        FiberAndThread(std::function<void()>* f, int thr)
            :thread(thr) {
            cb.swap(*f);
        }

        // 无参构造函数(stl 需要，不然无法进行初始化)
        FiberAndThread()
            :thread(-1) {
        }

        // 重置数据
        void reset() {
            fiber = nullptr;
            cb = nullptr;
            thread = -1;
        }
    };
private:  
    MutexType m_mutex;                      // 互斥量
    std::vector<Thread::ptr> m_threads;     // 线程池   
    std::list<FiberAndThread> m_fibers;     // 待执行的协程队列  
    Fiber::ptr m_rootFiber;                 // use_caller为true时有效, 调度协程   
    std::string m_name;                     // 协程调度器名称

protected:  
    std::vector<int> m_threadIds;                   // 协程下的线程id数组 
    size_t m_threadCount = 0;                       // 线程数量 
    std::atomic<size_t> m_activeThreadCount = {0};  // 工作线程数量，用原子量表示，减少加锁的操作
    std::atomic<size_t> m_idleThreadCount = {0};    // 空闲线程数量 
    bool m_stopping = true;                         // 是否正在停止  
    bool m_autoStop = false;                        // 是否自动停止  
    int m_rootThread = 0;                           // 主线程id(use_caller)
};

// class SchedulerSwitcher : public Noncopyable {
// public:
//     SchedulerSwitcher(Scheduler* target = nullptr);
//     ~SchedulerSwitcher();
// private:
//     Scheduler* m_caller;
// };

}

#endif