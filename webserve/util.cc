#include "util.h"
#include "log.h"
#include "fiber.h"
#include <execinfo.h>

namespace sylar{

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

// 获取线程 id
pid_t GetThreadId(){
    return syscall(SYS_gettid);
}

// 获取协程 id
uint32_t GetFiberId(){
    return sylar::Fiber::GetFiberId();
}

// 自定义 获取当前线程的栈信息，并存储到 vector bt 中
void Backtrace(std::vector<std::string>& bt, int size, int skip) {
    void** array = (void**)malloc((sizeof(void*) * size));
    // backtrace 获取当前线程的调用堆栈，获取的信息将会被存放在数组 array 中
    size_t s = ::backtrace(array, size);

    // 将线程的栈信息以 string 的形式输出
    char** strings = backtrace_symbols(array, s);
    if(strings == NULL) {
        SYLAR_LOG_ERROR(g_logger) << "backtrace_synbols error";
        return;
    }

    for(size_t i = skip; i < s; ++i) {
        // bt.push_back(demangle(strings[i]));
        bt.push_back(strings[i]);
    }

    free(strings);
    free(array);
}

// 输出当前线程的栈信息
std::string BacktraceToString(int size, int skip, const std::string& prefix) {
    std::vector<std::string> bt;
    Backtrace(bt, size, skip);
    std::stringstream ss;
    for(size_t i = 0; i < bt.size(); ++i) {
        ss << prefix << bt[i] << std::endl;
    }
    return ss.str();
}


}