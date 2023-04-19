#pragma once

#include "noncopyable.h"

class InetAddress;

// 封装socket fd
class Socket : noncopyable
{
public:
    explicit Socket(int sockfd) : sockfd_(sockfd)
    {
    }
    ~Socket();

    // 返回套接字文件描述符
    int fd() const { return sockfd_; }
    // 绑定本地地址
    void bindAddress(const InetAddress &localaddr);
    // 监听套接字
    void listen();
    // 接受新连接，并返回新连接的文件描述符
    int accept(InetAddress *peeraddr);

    // 关闭套接字的写端
    void shutdownWrite();

    // 设置或取消 TCP_NODELAY 选项
    void setTcpNoDelay(bool on);
    // 设置或取消 SO_REUSEADDR 选项
    void setReuseAddr(bool on);
    // 设置或取消 SO_REUSEPORT 选项
    void setReusePort(bool on);
    // 设置或取消 SO_KEEPALIVE 选项
    void setKeepAlive(bool on);

private:
    const int sockfd_;
};