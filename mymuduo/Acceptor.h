#pragma once

#include <functional>

#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"

class EventLoop;
class InetAddress;

// 实际上，Acceptor只是对Channel的封装，通过Channel关注listenfd的可读事件，并设置好回调函数就可以了
class Acceptor : noncopyable
{
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress &)>;

    Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport);
    ~Acceptor();

    // 设置新连接回调函数。当有新连接到来时，这个函数将被调用
    void setNewConnectionCallback(const NewConnectionCallback &cb) { NewConnectionCallback_ = cb; }

    // 返回一个布尔值，表示Acceptor是否正在监听
    bool listenning() const { return listenning_; }
    // 开始监听本地端口
    void listen();

private:
    void handleRead();

    EventLoop *loop_;                             // Acceptor用的就是用户定义的那个baseLoop 也称作mainLoop
    Socket acceptSocket_;                         // 用于接受新连接
    Channel acceptChannel_;                       // 用于处理与接受socket相关的事件
    NewConnectionCallback NewConnectionCallback_; // 是一个函数对象，当有新连接到来时，它将被调用
    bool listenning_;                             // 表示Acceptor是否正在监听
};