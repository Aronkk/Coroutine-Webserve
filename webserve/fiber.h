#ifndef __SYLAR_FIBER_H__
#define __SYLAR_FIBER_H__

#include <memory>
#include <functional>
#include <ucontext.h>

// 协程，类似一个可以暂停的函数
// ucontext.h 协程的关键特点是调度/挂起可以由开发者控制。协程比线程轻量的多。
// 在语言层面实现协程是让其内部有一个类似栈的数据结构，当该协程被挂起时能够保存该协程的数据现场以便恢复执行。
// 主要功能函数：getcontext，setcontext，makecontext，swapcontext
// 功能为：获取/设置切换/创建设置/保存并切换，某一个节点的上下文环境，用于多协程的并发运行

namespace sylar {

class Scheduler;
// 协程类，将其设置为智能指针类，自己调用自己
class Fiber : public std::enable_shared_from_this<Fiber> {
friend class Scheduler;
public:
    typedef std::shared_ptr<Fiber> ptr;
    // 协程状态
    enum State {        
        INIT,       // 初始化状态        
        HOLD,       // 暂停状态        
        EXEC,       // 执行中状态       
        TERM,       // 结束状态       
        READY,      // 可执行状态
        EXCEPT      // 异常状态
    };
private:
    // 无参构造函数, 每个线程第一个协程的构造
    // 将默认构造函数私有化，不允许这样构造
    Fiber();

public:
    /**
     * @brief 构造函数
     * @param[in] cb 协程执行的函数
     * @param[in] stacksize 协程栈大小
     * @param[in] use_caller 是否在MainFiber（主协程）上调度
     */
    Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);

    ~Fiber();

    /**
     * @brief 重置协程执行函数,并设置状态；或者当一个函数执行完了，可以分配执行另一个协程
     * @pre getState() 为 INIT, TERM, EXCEPT
     * @post getState() = INIT
     */
    void reset(std::function<void()> cb);

    /**
     * @brief 将当前协程切换到运行状态
     * @pre getState() != EXEC
     * @post getState() = EXEC
     */
    void swapIn();

    // 将当前协程切换到后台执行？
    void swapOut();

    // 将当前线程切换到执行状态，执行的为当前线程的主协程
    void call();

    // 将当前线程切换到后台，执行的为该协程，返回到线程的主协程
    void back();

    // 返回协程id
    uint64_t getId() const { return m_id;}

    // 返回协程状态
    State getState() const { return m_state;}
public:
    // 静态方法又叫类方法。属于类的，不属于对象，在实例化对象之前就可以通过类名.方法名调用静态方法。可以直接通过类名调用
    // 非静态方法又称为实例方法，成员方法。属于对象的，不属于类的。必须通过new关键字创建对象后，再通过对象调用
    
    // 设置当前线程的运行协程
    static void SetThis(Fiber* f);

    // 返回当前所在的协程
    static Fiber::ptr GetThis();
    
    // 将当前协程切换到后台,并设置为READY状态，getState() = READY
    static void YieldToReady();

    // 将当前协程切换到后台,并设置为HOLD状态，getState() = HOLD
    static void YieldToHold();

    // 返回当前协程的总数量
    static uint64_t TotalFibers();

    // 协程执行函数，执行完成返回到线程主协程
    static void MainFunc();

    /**
     * @brief 协程执行函数
     * @post 执行完成返回到线程调度协程
     */
    static void CallerMainFunc();

    // 获取当前协程的id，不同于 getthis()，因为有点线程不一定有协程，所以需要单独的一个方法
    static uint64_t GetFiberId();
private:
    uint64_t m_id = 0;              // 协程id
    uint32_t m_stacksize = 0;       // 协程运行栈大小  
    State m_state = INIT;           // 协程状态  
    ucontext_t m_ctx;               // 协程上下文 
    void* m_stack = nullptr;        // 协程运行栈指针 
    std::function<void()> m_cb;     // 协程运行函数
};

}

#endif