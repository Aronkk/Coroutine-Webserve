#ifndef __SYLAR_UTIL_H__
#define __SYLAR_UTIL_H__

#include <iostream>
#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdint.h>
#include <vector>
#include <string>

namespace sylar{

pid_t GetThreadId();
uint32_t GetFiberId();

/**
 * @brief 获取当前的调用栈
 * @param[out] bt 保存调用栈
 * @param[in] size 最多返回层数
 * @param[in] skip 跳过栈顶的层数
 */
void Backtrace(std::vector<std::string>& bt, int size = 64, int skip = 1);

/**
 * @brief 获取当前栈信息的字符串
 * @param[in] size 栈的最大层数
 * @param[in] skip 跳过栈顶的层数
 * @param[in] prefix 栈信息前输出的内容
 */
std::string BacktraceToString(int size = 64, int skip = 2, const std::string& prefix = "");

}

#endif