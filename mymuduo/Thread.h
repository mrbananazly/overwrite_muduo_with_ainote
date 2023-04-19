#pragma once

#include "noncopyable.h"

#include <functional>
#include <thread>
#include <memory>
#include <unistd.h>
#include <string>
#include <atomic>

class Thread : noncopyable
{
public:
    using ThreadFunc = std::function<void()>;

    explicit Thread(ThreadFunc func, const std::string &name = std::string());
    ~Thread();

    void start();
    void join();

    bool started() const { return started_; }
    pid_t tid() const { return tid_; }
    const std::string &name() const { return name_; }

    static int numCreated() { return numCreated_; }

private:
    void setDefaultName(); // 设置默认线程名字

    bool started_;                        // 标识线程是否已经启动
    bool joined_;                         // 表示线程是否被join()回收
    std::shared_ptr<std::thread> thread_; // 指向该线程的智能指针
    pid_t tid_;                           // 线程ID
    ThreadFunc func_;                     // 线程函数
    std::string name_;                    // 线程名字
    static std::atomic_int numCreated_;   // 被创建的线程数
};