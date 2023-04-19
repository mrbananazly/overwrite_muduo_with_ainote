#include <stdlib.h>

#include "Poller.h"
#include "EPollPoller.h"

Poller *Poller::newDefaultPoller(EventLoop *loop)
{
    if (::getenv("MUDUO_USE_POLL"))
    {
        // 生成poll的实例
        return nullptr;
    }
    else
    {
        // 生成epoll的实例
        return new EPollPoller(loop);
    }
}