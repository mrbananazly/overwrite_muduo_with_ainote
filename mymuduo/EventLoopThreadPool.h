#pragma once

#include <functional>
#include <string>
#include <vector>
#include <memory>

#include "noncopyable.h"

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;
    // 两个参数 指向EventLoop对象的指针；线程池名称
    EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg);
    ~EventLoopThreadPool();
    // 设置线程池中的线程数量
    void setThreadNum(int numThreads) { numThreads_ = numThreads; }
    // 启动线程池，并可选地接受一个回调函数作为参数
    void start(const ThreadInitCallback &cb = ThreadInitCallback());
    // 获取下一个事件循环对象
    EventLoop *getNextLoop();
    // 获取所有事件循环对象；如果工作在多线程中，baseLoop_(mainLoop)会默认以轮询的方式分配Channel给subLoop
    std::vector<EventLoop *> getAllLoops();
    // 返回线程池是否已启动
    bool started() const { return started_; }
    // 返回线程池的名称
    const std::string name() const { return name_; }

private:
    EventLoop *baseLoop_;                                   // 用户使用muduo创建的loop 如果线程数为1 那直接使用用户创建的loop 否则创建多EventLoop
    std::string name_;                                      // 线程池的名称
    bool started_;                                          // 线程池是否已启动
    int numThreads_;                                        // 线程池中线程的数量
    int next_;                                              // 轮询的下标
    std::vector<std::unique_ptr<EventLoopThread>> threads_; // 存储线程池中的线程
    std::vector<EventLoop *> loops_;                        // 存储事件循环对象
};