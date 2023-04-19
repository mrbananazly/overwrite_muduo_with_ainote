#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>

/**
 * one thread one loop 防止一个线程创建多个EventLoop
 * t_loopInThisThread是一个thread_local变量
 * 如果该变量非空，则说明当前线程已经有一个EventLoop对象了；否则，将当前对象指针赋值给该变量，表示当前线程绑定了这个 EventLoop 对象
 */
__thread EventLoop *t_loopInThisThread = nullptr;

// 定义默认的Poller IO复用接口的超时时间
const int kPollTimeMs = 10000;

int createEventfd()
{
    /**
     * 创建一个事件文件描述符
     * EFD_NONBLOCK：这是一个标志位，表示事件文件描述符应该以非阻塞方式打开，即在没有可用事件时不会阻塞进程
     * EFD_CLOEXEC：这也是一个标志位，表示在执行exec()函数时关闭事件文件描述符
     */
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG_FATAL("eventfd error:%d \n", errno);
    }
    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false), quit_(false), callingPendingFunctors_(false), threadId_(CurrentThread::tid()), poller_(Poller::newDefaultPoller(this)), wakeupFd_(createEventfd()), wakeupChannel_(new Channel(this, wakeupFd_))
{
    // 判断当前线程是否已经有一个EventLoop对象了
    LOG_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);
    if (t_loopInThisThread)
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n", t_loopInThisThread, threadId_);
    }
    else
    {
        t_loopInThisThread = this;
    }

    // 设置回调函数为EventLoop中的handleRead
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));

    // 启动EventLoop中的fd以便在该fd上进行读操作时触发相应的事件回调函数
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    // 关闭唤醒通道的文件描述符，释放相关资源
    ::close(wakeupFd_);
    // 将指向当前线程事件循环的指针置空，避免出现悬挂指针的情况
    t_loopInThisThread = nullptr;
}

// 开启事件循环
void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping \n", this);

    while (!quit_)
    {
        activeChannels_.clear();

        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        for (Channel *channel : activeChannels_)
        {
            channel->handleEvent(pollReturnTime_);
        }
        doPendingFunctors();
    }
    LOG_INFO("EventLoop %p stop looping. \n", this);
    looping_ = false;
}

/**
 * 退出事件循环
 * 1.loop在自己的线程中调用quit
 * 2.在非loop的线程中，调用loop的quit
 */
void EventLoop::quit()
{
    quit_ = true;
    // 若当前线程不为为EventLoop所在的线程，唤醒事件循环所在的线程，以便能够在合适的时候退出事件循环
    if (!isInLoopThread())
    {
        wakeup();
    }
}

// 在事件循环线程中执行回调函数
void EventLoop::runInLoop(Functor cb)
{
    // 检查当前线程是否与事件循环线程相同
    if (isInLoopThread())
    {
        // 直接执行回调函数
        cb();
    }
    else
    {
        // 将回调函数放入事件队列，等待事件循环线程进行处理
        queueInLoop(cb);
    }
}

// 将回调函数cb加入相应的队列中，唤醒loop所在的线程，执行cb
void EventLoop::queueInLoop(Functor cb)
{
    // 获取互斥锁 mutex_ 的所有权，确保只有当前线程可以对 pendingFunctors_ 队列进行操作
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }
    /**
     * 如果当前线程并非事件循环所在线程，或者当前正在执行待处理的回调函数，则调用 wakeup() 函数
     * 向该事件循环对应的线程发送唤醒信号，以便及时处理新加入的回调函数
     */
    if (!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup();
    }
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8", n);
    }
}

// 用来唤醒loop所在的线程的  向wakeupfd_写一个数据，wakeupChannel就发生读事件，当前loop线程就会被唤醒
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n", n);
    }
}

// EventLoop的方法 =》 Poller的方法
void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel)
{
    return poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;  // 创建一个存储回调函数对象的vector容器functors
    callingPendingFunctors_ = true; // 表示当前正在执行待处理的回调函数
    // 使用互斥锁(mutex_)保护，将事件循环队列(pendingFunctors_)中所有待处理的回调函数对象从主线程转移到functors容器中
    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (const Functor &functor : functors)
    {
        functor(); // 执行当前loop需要执行的回调操作
    }

    callingPendingFunctors_ = false;
}