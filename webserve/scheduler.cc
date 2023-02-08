#include "scheduler.h"
#include "log.h"
#include "macro.h"
// #include "hook.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

// 线程局部变量声明 -- 协程调度器的指针 t_scheduler；声明调度器的主协程 t_scheduler_fiber
static thread_local Scheduler* t_scheduler = nullptr;
static thread_local Fiber* t_scheduler_fiber = nullptr;

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name)
    :m_name(name) {
    SYLAR_ASSERT(threads > 0);

    // use_caller 是否使用当前调用线程，true 说明该线程可以被调度
    if(use_caller) {
        // 如果当前线程有协程，那就读取；如果没有那就生成（nullptr）-- 主协程
        sylar::Fiber::GetThis();
        --threads;      // 因为当前线程可以被调度，所以需要创建的线程数 -1

        // 一个线程只能有一个协程调度器，因此要判断GetThis()得到的线程是不是有了调度器
        SYLAR_ASSERT(GetThis() == nullptr);
        t_scheduler = this;

        // 调度器（scheduler）内部会初始化一个属于caller线程的调度协程并保存起来
        // 这个地方的关键点在于，是否把创建协程调度器的线程放到协程调度器管理的线程池中。
        // 如果不放入，那这个线程专职协程调度；如果放的话，那就要把协程调度器封装到一个协程中，称之为主协程或协程调度器协程。
        // 这里相当于是新建一个协程，用来处理任务，这个协程就是线程的主协程
        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
        sylar::Thread::SetName(m_name);



        // scheduler线程生成了一个调度器（协程），然后再把该线程放到调度器里去，
        // 此时该线程的主协程并不是这个调度器，而且上面新建的这个协程
        t_scheduler_fiber = m_rootFiber.get();
        m_rootThread = sylar::GetThreadId();
        m_threadIds.push_back(m_rootThread);
    } else {
        // 主线程专职协程调度，而不执行任务
        m_rootThread = -1;
    }
    m_threadCount = threads;
}

Scheduler::~Scheduler() {
    SYLAR_ASSERT(m_stopping);
    if(GetThis() == this) {
        t_scheduler = nullptr;
    }
}

Scheduler* Scheduler::GetThis() {
    return t_scheduler;
}

Fiber* Scheduler::GetMainFiber() {
    return t_scheduler_fiber;
}

// start：生成线程池
void Scheduler::start() {
    MutexType::Lock lock(m_mutex);
    // 如果现在是运行状态，那就不需要启动，直接return
    if(!m_stopping) {
        return;
    }
    m_stopping = false;
    SYLAR_ASSERT(m_threads.empty());

    // m_threads--线程池，vector类型，这里给它重新分配大小
    m_threads.resize(m_threadCount);
    // 生成线程池
    // std::cout<<m_threadCount<<std::endl;
    for(size_t i = 0; i < m_threadCount; ++i) {
        // bind--绑定，在生成线程时，运行Scheduler里的run函数
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this)
                            , m_name + "_" + std::to_string(i)));
        m_threadIds.push_back(m_threads[i]->getId());
        // std::cout << m_threads[i]->getId() <<std::endl;
    }
    lock.unlock();

    // if(m_rootFiber) {
    //    //m_rootFiber->swapIn();
    //    m_rootFiber->call();
    //    SYLAR_LOG_INFO(g_logger) << "call out " << m_rootFiber->getState();
    // }
}

// stop：当协程不在运行状态时，需要进入循环等待的状态
void Scheduler::stop() {
    m_autoStop = true;
    if(m_rootFiber
            && m_threadCount == 0
            && (m_rootFiber->getState() == Fiber::TERM
                || m_rootFiber->getState() == Fiber::INIT)) {
        // 这里说明，协程是真的已经停止了
        SYLAR_LOG_INFO(g_logger) << this << " stopped";
        m_stopping = true;

        // stopping 是留给子类去实现的
        if(stopping()) {
            return;
        }
    }

    //bool exit_on_this_fiber = false;
    // m_rootThread（主线程）!= -1，说明该线程可以被调度
    if(m_rootThread != -1) {
        SYLAR_ASSERT(GetThis() == this);
    } else {
        SYLAR_ASSERT(GetThis() != this);
    }

    // 线程停止，通知所有的线程结束
    m_stopping = true;
    for(size_t i = 0; i < m_threadCount; ++i) {
        tickle();
    }

    if(m_rootFiber) {
        tickle();
    }

    // 这里是判断是否有 m_rootFiber（nullptr）
    if(m_rootFiber) {
        //while(!stopping()) {
        //    if(m_rootFiber->getState() == Fiber::TERM
        //            || m_rootFiber->getState() == Fiber::EXCEPT) {
        //        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
        //        SYLAR_LOG_INFO(g_logger) << " root fiber is term, reset";
        //        t_fiber = m_rootFiber.get();
        //    }
        //    m_rootFiber->call();
        //}
        if(!stopping()) {
            m_rootFiber->call();
        }
    }

    std::vector<Thread::ptr> thrs;
    {
        MutexType::Lock lock(m_mutex);
        thrs.swap(m_threads);
    }

    for(auto& i : thrs) {
        i->join();
    }
    //if(exit_on_this_fiber) {
    //}
}

void Scheduler::setThis() {
    t_scheduler = this;
}

// 线程进行 run，从协程队列里取协程执行任务 
void Scheduler::run() {
    SYLAR_LOG_DEBUG(g_logger) << m_name << " run";
    // set_hook_enable(true);
    // 获取当前线程的协程调度器 -- t_scheduler
    setThis();
    // std::cout<<"hello world  "<<sylar::GetFiberId()<<std::endl;
    // 如果当前线程的id != 主线程的id；那么调度器的主协程 = 当前线程的主协程？
    if(sylar::GetThreadId() != m_rootThread) {
        t_scheduler_fiber = Fiber::GetThis().get();
    }

    // idle_fiber -- 协程没有任务调度时执行idle协程，cb_fiber -- 回调函数的协程
    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    Fiber::ptr cb_fiber;

    // ft 就是下面取出来的任务协程
    FiberAndThread ft;
    while(true) {
        ft.reset();
        bool tickle_me = false;
        bool is_active = false;
        {
            MutexType::Lock lock(m_mutex);
            // 从协程消息队列（list）中取出一个合适的协程
            auto it = m_fibers.begin();
            while(it != m_fibers.end()) {
                // 如果任务已经指定由某个线程执行，或者run的线程不是当前线程，那就忽略不执行
                // 因为协程的唤醒是抢占的，抢到信号的那个协程不一定是我们需要的，因此还需要通知其他协程继续抢占
                // 为什么要指定协程，不应该是哪个协程空闲哪个就去执行吗？
                if(it->thread != -1 && it->thread != sylar::GetThreadId()) {
                    ++it;
                    tickle_me = true;
                    continue;
                }

                SYLAR_ASSERT(it->fiber || it->cb);
                // 为什么还会有正在执行的状态啊？
                if(it->fiber && it->fiber->getState() == Fiber::EXEC) {
                    ++it;
                    continue;
                }

                ft = *it;
                m_fibers.erase(it++);
                ++m_activeThreadCount;
                is_active = true;
                break;
            }
            // 这一行是啥意思啊？
            tickle_me |= it != m_fibers.end();
        }
        
        // 从 while 循环出来就说明已经找到了协程，下面就是执行函数
        if(tickle_me) {
            tickle();
        }

        // 如果该协程不是结束/异常状态（这种状态为什么还放入队列里）就唤醒执行
        if(ft.fiber && (ft.fiber->getState() != Fiber::TERM
                        && ft.fiber->getState() != Fiber::EXCEPT)) {
            ft.fiber->swapIn();
            --m_activeThreadCount;

            // 执行之后的状态判断，如果是ready就继续执行
            if(ft.fiber->getState() == Fiber::READY) {
                schedule(ft.fiber);
            } else if(ft.fiber->getState() != Fiber::TERM
                    && ft.fiber->getState() != Fiber::EXCEPT) {
                ft.fiber->m_state = Fiber::HOLD;
            }
            ft.reset();
        } else if(ft.cb) {
            // 如果取出来的是一个包装成协程的cb函数，后面跟上面的协程判断差不多
            if(cb_fiber) {
                cb_fiber->reset(ft.cb);
            } else {
                cb_fiber.reset(new Fiber(ft.cb));
            }
            ft.reset();
            cb_fiber->swapIn();
            --m_activeThreadCount;

            if(cb_fiber->getState() == Fiber::READY) {
                schedule(cb_fiber);
                cb_fiber.reset();
                // 为什么函数的这些状态不需要变成 hold ？
            } else if(cb_fiber->getState() == Fiber::EXCEPT
                    || cb_fiber->getState() == Fiber::TERM) {
                cb_fiber->reset(nullptr);
            } else {//if(cb_fiber->getState() != Fiber::TERM) {
                cb_fiber->m_state = Fiber::HOLD;
                cb_fiber.reset();
            }
        } else {
            // 如果没有任务可以做，这里就执行 idle
            if(is_active) {
                --m_activeThreadCount;
                continue;
            }
            if(idle_fiber->getState() == Fiber::TERM) {
                SYLAR_LOG_INFO(g_logger) << "idle fiber term";
                break;
            }

            ++m_idleThreadCount;
            idle_fiber->swapIn();
            --m_idleThreadCount;
            if(idle_fiber->getState() != Fiber::TERM
                    && idle_fiber->getState() != Fiber::EXCEPT) {
                idle_fiber->m_state = Fiber::HOLD;
            }
        }
    }
}

// 唤醒线程
void Scheduler::tickle() {
    SYLAR_LOG_INFO(g_logger) << "tickle";
}

bool Scheduler::stopping() {
    MutexType::Lock lock(m_mutex);
    return m_autoStop && m_stopping
        && m_fibers.empty() && m_activeThreadCount == 0;
}

void Scheduler::idle() {
    SYLAR_LOG_INFO(g_logger) << "idle";
    while(!stopping()) {
        sylar::Fiber::YieldToHold();
    }
}

void Scheduler::switchTo(int thread) {
    SYLAR_ASSERT(Scheduler::GetThis() != nullptr);
    if(Scheduler::GetThis() == this) {
        if(thread == -1 || thread == sylar::GetThreadId()) {
            return;
        }
    }
    schedule(Fiber::GetThis(), thread);
    Fiber::YieldToHold();
}

std::ostream& Scheduler::dump(std::ostream& os) {
    os << "[Scheduler name=" << m_name
       << " size=" << m_threadCount
       << " active_count=" << m_activeThreadCount
       << " idle_count=" << m_idleThreadCount
       << " stopping=" << m_stopping
       << " ]" << std::endl << "    ";
    for(size_t i = 0; i < m_threadIds.size(); ++i) {
        if(i) {
            os << ", ";
        }
        os << m_threadIds[i];
    }
    return os;
}

// SchedulerSwitcher::SchedulerSwitcher(Scheduler* target) {
//     m_caller = Scheduler::GetThis();
//     if(target) {
//         target->switchTo();
//     }
// }

// SchedulerSwitcher::~SchedulerSwitcher() {
//     if(m_caller) {
//         m_caller->switchTo();
//     }
// }

}
