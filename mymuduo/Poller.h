#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <vector>
#include <unordered_map>

class Channel;
class EventLoop;
// muduo库中多路事件分发器的核心IO复用模块
class Poller : noncopyable
{
public:
    using ChannelList = std::vector<Channel *>;
    Poller(EventLoop *Loop);
    virtual ~Poller() = default;

    // 给所有IO复用保留统一的接口
    // 等待事件的发生，并返回发生事件的时间戳，当有事件发生时，需要将相关的通道加入到activeChannels中
    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
    // 更新指定的通道在Poller中的状态
    virtual void updateChannel(Channel *channel) = 0;
    // 将指定的通道从Poller中移除
    virtual void removeChannel(Channel *channel) = 0;

    // 判断参数channel是否在当前poller当中
    bool hasChannel(Channel *channel) const;

    // EventLoop可以通过该接口获取默认的IO复用的具体实现
    static Poller *newDefaultPoller(EventLoop *loop);

protected:
    // ChannelMap => [sockfd, sockfd所属的channel通道类型]
    using ChannelMap = std::unordered_map<int, Channel *>;
    ChannelMap channels_;

private:
    // 定义Poller所属的事件循环EventLoop
    EventLoop *ownerLoop_;
};