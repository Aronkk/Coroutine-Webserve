#include "webserve/sylar.h"
#include "webserve/util.h"
#include "webserve/macro.h"
#include <assert.h>

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_assert(){
    SYLAR_LOG_INFO(g_logger) << sylar::BacktraceToString(10);
    // SYLAR_ASSERT(false);
    SYLAR_ASSERT2(0 == 1, "abcdefg");
}

int main(int argc, char** argv){
    test_assert();
    return 0;
}