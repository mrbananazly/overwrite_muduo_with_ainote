#pragma once

#include <unistd.h>
#include <sys/syscall.h>

namespace CurrentThread
{
    // 外部的、线程局部的整型变量
    extern __thread int t_cachedTid;

    // 用于缓存当前线程的ID
    void cacheTid();

    // 返回当前线程ID
    inline int tid()
    {
        // GCC内置的一种优化手段，用于提示编译器某些分支的概率，从而帮助编译器生成更加高效的代码
        // 提示编译器，t_cachedTid等于0的情况发生的概率很小，从而让编译器在生成汇编代码时做出相应的优化
        if (__builtin_expect(t_cachedTid == 0, 0))
        {
            cacheTid();
        }
        return t_cachedTid;
    }
}