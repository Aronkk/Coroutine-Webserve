#include "webserve/sylar.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void run_in_fiber() {
    SYLAR_LOG_INFO(g_logger) << "run_in_fiber begin";
    sylar::Fiber::YieldToHold();    // （4）子协程切换到后台（hold），返回执行主协程

    SYLAR_LOG_INFO(g_logger) << "run_in_fiber end";
    sylar::Fiber::YieldToHold();    // （6）子协程切换到后台（hold），返回执行主协程
}

void test_fiber() {
    SYLAR_LOG_INFO(g_logger) << "main begin -1";
    {
        sylar::Fiber::GetThis();        // （1）初始化，创建一个 main_fiber
        SYLAR_LOG_INFO(g_logger) << "main begin";

        // （2）构造一个子协程sub_fiber（调用 run_in_fiber 函数）
        sylar::Fiber::ptr fiber(new sylar::Fiber(run_in_fiber));
        fiber->swapIn();    // （3）执行子协程 sub_fiber

        SYLAR_LOG_INFO(g_logger) << "main after swapIn";
        fiber->swapIn();    // （5）再执行子协程 sub_fiber

        SYLAR_LOG_INFO(g_logger) << "main after end";
        fiber->swapIn();    // （7）再执行子协程 sub_fiber    
    }
    SYLAR_LOG_INFO(g_logger) << "main after end2";      // （8）函数结束，析构子协程和主协程
}

int main(int argc, char** argv) {
    sylar::Thread::SetName("main");

    // 创建 3 个线程，然后多线程的协程进行切换，join 自动释放线程
    std::vector<sylar::Thread::ptr> thrs;
    for(int i = 0; i < 3; ++i) {
        thrs.push_back(sylar::Thread::ptr(
                    new sylar::Thread(&test_fiber, "name_" + std::to_string(i))));
    }
    for(auto i : thrs) {
        i->join();
    }
    return 0;
}
