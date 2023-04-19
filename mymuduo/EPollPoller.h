#pragma once

#include "Poller.h"

#include <vector>
#include <sys/epoll.h>

/**
 * epoll的使用
 * 1. epoll_create
 * 2. epoll_etl (add mod del)
 * 3. epoll_wait
 */

class EPollPoller : public Poller
{
public:
    EPollPoller(EventLoop *loop);
    ~EPollPoller() override;

    // 重写基类Poller的抽象方法
    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
    void updateChannel(Channel *channel) override;
    void removeChannel(Channel *channel) override;

private:
    static const int kInitEventListSize = 16;

    // 填写活跃的连接
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    // 更新channel通道
    void update(int operation, Channel *channel);

    // C++中可以省略struct 直接写epoll_event即可
    using EventList = std::vector<epoll_event>;

    // epoll_create创建返回的fd保存在epollfd_中
    int epollfd_;
    // 用于存放epoll_wait返回的所有发生的事件的文件描述符事件集
    EventList events_;
};