#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <functional>
#include <memory>

// 前置声明 尽量避免头文件暴露信息
class EventLoop;

/**
 * 理清楚 EventLoop Channel Poller之间的关系   《=  Reactor 模型下对应 Demultiplex
 * Channel 理解为通道，封装了sockfd和其感兴趣的event，如EPOLLIN , EPOLLOUT事件
 * 还绑定了poller返回的具体事件
 */
class Channel : noncopyable
{
public:
    // 两个回调函数
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;
    Channel(EventLoop *loop, int fd);
    ~Channel();
    // fd得到poller通知以后，处理时间的
    void handleEvent(Timestamp receiveTime);

    // 设置回调函数对象
    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

    // 防止当channel被手动remove掉，channel还在执行回调操作
    void tie(const std::shared_ptr<void> &);

    int fd() const { return fd_; }
    int events() const { return events_; }
    // Channel本身无法监听自身事件，需要epoll监听进行设置
    int set_revents(int revt) { revents_ = revt; }

    // 设置fd相应的事件状态
    void enableReading()
    {
        events_ |= kReadEvent;
        update();
    }
    void disableReading()
    {
        events_ &= ~kReadEvent;
        update();
    }
    void enableWriting()
    {
        events_ |= kWriteEvent;
        update();
    }
    void disableWriting()
    {
        events_ &= ~kWriteEvent;
        update();
    }
    // 禁用唤醒通道(wakeup channel)上的所有事件，确保不再有新的事件被触发
    void disableAll()
    {
        events_ = kNoneEvent;
        update();
    }

    // 返回fd当前的事件状态
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; }

    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    // one loop one thread
    EventLoop *ownerLoop() { return loop_; }
    // 将唤醒通道从事件循环中移除，即不再监控该通道上的任何事件
    void remove();

private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime);
    // 表示当前fd状态：不对事件感兴趣；对读事件感兴趣；对写事件感兴趣
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_; // 事件循环
    const int fd_;    // fd, Poller监听的对象
    int events_;      // 注册fd感兴趣的事件
    int revents_;     // poller返回的具体发生的事件
    int index_;       // poller使用

    std::weak_ptr<void> tie_; // 弱智能指针
    bool tied_;               // 是否被绑定

    // 因为channel通道里面能够获知fd最终发生的具体事件revents，所以它负责调用具体事件的回调操作
    // 四个函数对象，可绑定外部传入的操作，这些回调具体操作是外部用户指定的
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};