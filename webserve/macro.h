#ifndef __SYLAR_MACRO_H__
#define __SYLAR_MACRO_H__

#include <string.h>
#include <assert.h>
#include "log.h"
#include "util.h"

// assert 是用于对程序进行调试的，对于执行结构的判断，而不是对于业务流程的判断。
// 断言只适用复杂的调式过程。（如果不复杂完全可以用log或者debug代替）
// assert需要自行开启，然后assert不具有继承性

/// 断言宏封装，输出错误信息所在的栈（无参）
#define SYLAR_ASSERT(x) \
    if(!(x)) { \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #x \
            << "\nbacktrace:\n" \
            << sylar::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }

/// 断言宏封装，输出错误信息所在的栈（有参）
#define SYLAR_ASSERT2(x, w) \
    if(!(x)) { \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #x \
            << "\n" << w \
            << "\nbacktrace:\n" \
            << sylar::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }

#endif