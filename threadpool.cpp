#include "threadpool.h"
#include <string.h>

int ThreadPool::m_maxThreads;
int ThreadPool::m_freeThreads;
int ThreadPool::m_initFlag = -1;
int ThreadPool::m_pushIndex = 0;
int ThreadPool::m_readIndex = 0;
int ThreadPool::m_size = 0;
std::vector<ThreadPool::Thread> ThreadPool::m_threadsQueue;
int ThreadPool::m_maxTasks;
std::vector<ThreadPool::Task> ThreadPool::m_taskQueue;
std::mutex ThreadPool::m_mutex;
std::condition_variable ThreadPool::m_cond;
bool ThreadPool::init(int threadsNum, int tasksNum)
{
    // 如果已经初始化，则返回true
    if (m_initFlag != -1)
    {
        return true;
    }
    // 设置最大线程数
    m_maxThreads = threadsNum;
    // 设置最大任务数
    m_maxTasks = tasksNum;
    // 初始化线程队列
    m_threadsQueue.resize(m_maxThreads);
    // 初始化空闲线程数
    m_freeThreads = m_maxThreads;
    // 初始化任务队列
    m_taskQueue.resize(m_maxTasks);
    // 初始化线程
    for (int i = 0; i < m_maxThreads; i++)
    {
        // 初始化线程终止标志
        m_threadsQueue[i].isTerminate = false;
        // 初始化线程忙碌标志
        m_threadsQueue[i].isWorking = false;
        // 创建线程
        std::thread *_thread = new std::thread(threadEventLoop,
                                               (void *)&m_threadsQueue[i]);
        // 如果创建失败，则返回false
        if (!_thread)
        {
            return false;
        }
        // 获取线程id
        m_threadsQueue[i].id = _thread->get_id();
        // 分离线程，使其独立运行
        _thread->detach();
    }
    // 初始化完成标志
    m_initFlag = 1;

    return true;
}

bool ThreadPool::addTask(std::function<void(std::shared_ptr<void>)> func, std::shared_ptr<void> par)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_size >= m_maxTasks)
    {
        return false;
    }
    m_taskQueue.at(m_pushIndex).func = func;
    m_taskQueue.at(m_pushIndex).par = par;
    m_size++;
    m_pushIndex = (m_pushIndex + 1) % m_maxTasks;
    m_cond.notify_one();
    return true;
}

void ThreadPool::releasePool()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    for (int i = 0; i < m_maxThreads; i++)
    {
        m_threadsQueue[i].isTerminate = true;
    }
    m_cond.notify_all();
    lock.unlock();
    // 等待线程全部退出
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

void ThreadPool::threadEventLoop(void *arg)
{
    auto theThread = reinterpret_cast<Thread *>(arg);
    // 循环执行任务
    while (true)
    {
        // 获取锁
        std::unique_lock<std::mutex> lock(m_mutex);
        // 当没有任务时，等待终止循环
        while (!m_size)
        {
            // 如果线程被终止，则终止循环
            if (theThread->isTerminate)
            {

                break;
            }
            m_cond.wait(lock);
        }
        // 如果线程被终止，则终止循环
        if (theThread->isTerminate)
        {
            break;
        }
        // 从队列中获取任务
        Task task = m_taskQueue[m_readIndex];
        // 任务处理完毕，将任务状态置空
        m_taskQueue[m_readIndex].func = nullptr;
        m_taskQueue[m_readIndex].par.reset();
        m_readIndex = (m_readIndex + 1) % m_maxTasks;
        // 任务数量减一
        m_size--;

        // 任务数量减一
        m_freeThreads--;
        lock.unlock();
        // 任务处理完毕，释放锁
        theThread->isWorking = true;
        task.func(task.par);
        printf("thread release\n");
        theThread->isWorking = false;
        lock.lock();
        // 任务处理完毕，释放锁
        m_freeThreads++;
    }
}