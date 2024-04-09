#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <thread>
#include <queue>
#include <mutex>
#include <functional>
#include <condition_variable>
class ThreadPool
{
public:
    static bool init(int threadsNum = 6, int tasksNum = 10);

    static bool addTask(std::function<void(std::shared_ptr<void>)> func, std::shared_ptr<void> par);

    static void releasePool();

public:
    typedef struct Task
    {
        // 任务函数
        std::function<void(std::shared_ptr<void>)> func;
        // 任务函数参数
        std::shared_ptr<void> par;
    } Task;

    typedef struct Thread
    {
        std::thread::id id;
        bool isTerminate;
        bool isWorking;
    } Thread;

    static void threadEventLoop(void *arg);
    static int m_maxThreads;
    static int m_freeThreads;
    static int m_maxTasks;
    static int m_pushIndex;
    static int m_readIndex;
    static int m_size;

    // 初始化标志，保证init首次调用有效
    static int m_initFlag;
    static std::vector<Thread> m_threadsQueue;
    static std::vector<Task> m_taskQueue;
    static std::mutex m_mutex;
    static std::condition_variable m_cond;
};

#endif