#include "Thread.h"
#include "CurrentThread.h"

#include <semaphore.h>

// 将被创建的线程数初始化为0
std::atomic_int Thread::numCreated_(0);

// func_(std::move(func))：将传入的 func 参数强制转换为右值引用，并移动到 func_ 成员变量中。这里使用了 std::move() 函数，以实现移动语义而非拷贝语义，提高代码效率
Thread::Thread(ThreadFunc func, const std::string &name)
    : started_(false), joined_(false), tid_(0), func_(std::move(func)), name_(name)
{
    setDefaultName();
}

/**
 * 如果线程已经启动但未被 join，则调用 std::thread 类的 detach() 方法将该线程设置为分离状态，从而使得线程资源可以自行释放。
 * 注意，如果一个线程被设置为分离状态，则不能再通过 join() 方法等待它结束，并且没有任何方法可以检查它是否已经结束或者获取它的返回值。
 */
Thread::~Thread()
{
    if (started_ && !joined_)
    {
        thread_->detach(); // thread类提供的设置分离线程的方法
    }
}

void Thread::start() // 一个Thread对象，记录的就是一个新线程的详细信息
{
    // 标记为已经启动
    started_ = true;
    // 创建一个信号量 sem 并初始化为 0
    sem_t sem;
    sem_init(&sem, false, 0);
    // 开启线程
    thread_ = std::shared_ptr<std::thread>(new std::thread([&]()
                                                           {
            // 获取当前线程的tid值
            tid_ = CurrentThread::tid();
            // 增加信号量的值，以通知主线程新线程已经启动并获取了 tid_ 的值
            sem_post(&sem);
            // 开启一个新线程，专门执行该线程函数
            func_(); }));
    // 等待信号量 sem 的值变为大于 0，表示新线程已经启动并且成功获取了 tid_ 的值
    sem_wait(&sem);
}

/**
 * 等待线程结束，并清理掉线程对象占用的资源
 * 注意，线程对象必须被 join 后才能安全地销毁，否则可能会发生竞争条件和数据竞争等问题。
 * */
void Thread::join()
{
    joined_ = true;
    thread_->join();
}

void Thread::setDefaultName()
{
    // 使用静态成员变量 numCreated_ 记录已经创建的线程数目，并将当前值加 1，作为新线程的编号
    int num = ++numCreated_;
    // 如果用户没有指定线程名称，则采用默认的线程名称
    if (name_.empty())
    {
        // 使用 snprintf() 函数将线程编号格式化为字符串，并存储到缓冲区 buf 中
        char buf[32] = {0};
        snprintf(buf, sizeof buf, "Thread%d", num);
        // 将缓冲区中的字符串赋值给线程名称
        name_ = buf;
    }
}