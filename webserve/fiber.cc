#include "fiber.h"
#include "config.h"
#include "macro.h"
#include "log.h"
#include "scheduler.h"
#include <atomic>

namespace sylar{

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

// 用全局原子量来计数，id和总数
static std::atomic<uint64_t> s_fiber_id {0};
static std::atomic<uint64_t> s_fiber_count {0};


// t_fiber 线程局部变量，当前线程正在运行的协程（main_fiber或者sub_fiber)
// t_threadFiber 是智能指针，当前线程的主协程，切换到这个协程，就相当于切换到了主线程中运行（指针有利于设置/切换/回收 sub_fiber）
static thread_local Fiber* t_fiber = nullptr;
static thread_local Fiber::ptr t_threadFiber = nullptr;     // 主要用于call和back函数


// 用配置文件来定义协程的栈大小
static ConfigVar<uint32_t>::ptr g_fiber_stack_size =
    Config::Lookup<uint32_t>("fiber.stack_size", 128 * 1024, "fiber stack size");


// 内存分配器，协程内存的大小是可以用户自定义的
class MallocStackAllocator {
public:
    // 分配内存
    static void* Alloc(size_t size) {
        return malloc(size);
    }

    // 回收内存
    static void Dealloc(void* vp, size_t size) {
        return free(vp);
    }
};

using StackAllocator = MallocStackAllocator;

uint64_t Fiber::GetFiberId() {
    if(t_fiber) {
        return t_fiber->getId();
    }
    return 0;
}

// 将线程的上下文设置成 main_fiber 的上下文
// main_fiber 的栈空间和协程id都是 0，并且状态一直都是执行中 EXEC
// 私有方法，不允许在类外部调用，只能通过GetThis()方法
Fiber::Fiber() {
    m_state = EXEC;
    SetThis(this);      // 设置当前线程的运行协程

    if(getcontext(&m_ctx)) {        // 获得当前协程的上下文
        SYLAR_ASSERT2(false, "getcontext");
    }

    ++s_fiber_count;
    SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber main";
}

// 创建一个协程，需要分配栈空间，具有回调函数
Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller)
    :m_id(++s_fiber_id)
    ,m_cb(cb) {
    ++s_fiber_count;
    // 栈的大小，如果是0，那就按配置的大小来分配；如果不是0，那给多少就是多少
    m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();


    // 栈的生成
    m_stack = StackAllocator::Alloc(m_stacksize);
    if(getcontext(&m_ctx)) {
        // assert 判断获取上下文是否成功
        SYLAR_ASSERT2(false, "getcontext");
    }
    // uc_link 保存当前函数结束后继续执行的函数，如果设为NULL，则表示当前结束后进程退出
    m_ctx.uc_link = nullptr;
    // uc_stack 制定协程的栈地址和大小
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    //makecontext 实现函数调用，这里判断是 main_fiber 还是 sub_fiber
    if(!use_caller) {
        makecontext(&m_ctx, &Fiber::MainFunc, 0);
    } else {
        makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
    }

    SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber id=" << m_id;
}

// 回收协程
Fiber::~Fiber() {
    --s_fiber_count;
    if(m_stack) {
        // 协程能被析构的状态
        SYLAR_ASSERT(m_state == TERM
                || m_state == EXCEPT
                || m_state == INIT);

        StackAllocator::Dealloc(m_stack, m_stacksize);
    } else {
        // 没有栈空间，那就是 main_fiber
        SYLAR_ASSERT(!m_cb);
        SYLAR_ASSERT(m_state == EXEC);

        Fiber* cur = t_fiber;
        if(cur == this) {
            SetThis(nullptr);
        }
    }
    SYLAR_LOG_DEBUG(g_logger) << "Fiber::~Fiber id=" << m_id
                              << " total=" << s_fiber_count;
}

// 重置协程函数（协程状态为INIT，TERM, EXCEPT，才能重置），并重置状态，
// 一个函数在执行完后，不回收其内存空间，而是初始化之后重新指向其他的协程
void Fiber::reset(std::function<void()> cb) {
    // 函数要reset，首先栈不能为空（不能是 main_fiber），其次状态为终止/异常/初始化
    SYLAR_ASSERT(m_stack);
    SYLAR_ASSERT(m_state == TERM
            || m_state == EXCEPT
            || m_state == INIT);
    m_cb = cb;

    // 获取协程的上下文环境，然后进行重置，和上面构造栈的内容一样
    if(getcontext(&m_ctx)) {
        SYLAR_ASSERT2(false, "getcontext");
    }
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;
    
    // 保存并切换到新的协程，状态为初始化 INIT
    makecontext(&m_ctx, &Fiber::MainFunc, 0);
    m_state = INIT;
}

// 将当前线程切换到执行状态，切换掉的是主协程，特殊化的swapin
void Fiber::call() {
    SetThis(this);
    m_state = EXEC;
    // t_threadFiber 智能指针--当前线程的主协程
    if(swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
        SYLAR_ASSERT2(false, "swapcontext");
    }
}

// 特殊化的swapout
void Fiber::back() {
    SetThis(t_threadFiber.get());
    if(swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
        SYLAR_ASSERT2(false, "swapcontext");
    }
}

// 将目标协程（sub_fiber)切换到当前协程，并执行
void Fiber::swapIn() {
    SetThis(this);
    SYLAR_ASSERT(m_state != EXEC);
    m_state = EXEC;

    // swapcontext 保存第一个协程的环境，切换到第二个协程的环境
    // 切换掉的是协程调度器的主协程 t_scheduler_fiber
    if(swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx)) 
    {
        SYLAR_ASSERT2(false, "swapcontext");
    }
}

// 切换到后台执行
void Fiber::swapOut() {
    SetThis(Scheduler::GetMainFiber());
    if(swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) 
    {
        SYLAR_ASSERT2(false, "swapcontext");
    }
}

// 设置当前协程
void Fiber::SetThis(Fiber* f) {
    t_fiber = f;
}

// 返回当前正在执行的协程(智能指针)，默认构造
// 该协程为当前线程的主协程，其他协程都通过这个协程来调度，也就是说，其他协程
// 结束时,都要切回到主协程，由主协程重新选择新的协程进行resume
Fiber::ptr Fiber:: GetThis() {
    if(t_fiber) {
        return t_fiber->shared_from_this();
    }
    // 如果没有主协程，那就 new 一个，并且判断一下全局和局部是否相同
    Fiber::ptr main_fiber(new Fiber);
    SYLAR_ASSERT(t_fiber == main_fiber.get());
    t_threadFiber = main_fiber;
    return t_fiber->shared_from_this();
}

// 将当前协程切换到后台，并且设置为Ready状态，现在执行的就是 main_fiber
void Fiber::YieldToReady() {
    Fiber::ptr cur = GetThis();
    SYLAR_ASSERT(cur->m_state == EXEC);
    cur->m_state = READY;
    cur->swapOut();
}

// 协程切换到后台，并且设置为Hold状态
void Fiber::YieldToHold() {
    Fiber::ptr cur = GetThis();
    SYLAR_ASSERT(cur->m_state == EXEC);
    cur->m_state = HOLD;
    cur->swapOut();
}

// 总协程数
uint64_t Fiber::TotalFibers() {
    return s_fiber_count;
}

// 协程的执行函数，在创建协程的时候就会调用这个函数
void Fiber::MainFunc() {
    Fiber::ptr cur = GetThis();     // 获取当前的协程
    SYLAR_ASSERT(cur);
    // 首先执行try中的代码 如果抛出异常会由catch去捕获并执行
    try {
        cur->m_cb();
        cur->m_cb = nullptr;    // 执行完后，要将cb中的参数还原（避免引用计数重复++） 
        cur->m_state = TERM;
    } catch (std::exception& ex) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
            << " fiber_id=" << cur->getId()
            << std::endl
            << sylar::BacktraceToString();
    } catch (...) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except"
            << " fiber_id=" << cur->getId()
            << std::endl
            << sylar::BacktraceToString();
    }

    // 子线程执行完了会自动回到主线程，但是子协程不会，因此需要手动返回 
    // 获取当前协程的指针，先释放掉，再进行 swapout 切换。（这里不就相当是野指针吗）
    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->swapOut();

    SYLAR_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}

void Fiber::CallerMainFunc() {
    Fiber::ptr cur = GetThis();
    SYLAR_ASSERT(cur);
    try {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    } catch (std::exception& ex) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
            << " fiber_id=" << cur->getId()
            << std::endl
            << sylar::BacktraceToString();
    } catch (...) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except"
            << " fiber_id=" << cur->getId()
            << std::endl
            << sylar::BacktraceToString();
    }

    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->back();
    SYLAR_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));

}

}