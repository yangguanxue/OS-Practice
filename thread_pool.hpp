#ifndef THREAD_POOL_H
#define THREAD_POOL_H
#include "sync_queue.hpp"
#include <iostream>
#include <vector>
#include <list>
#include <functional>
#include <memory>
#include <atomic>
#include <thread>

class ThreadPool
{
public:
    using Task = std::function<void()>;

    ThreadPool(int threadNum = std::thread::hardware_concurrency(), int maxTaskNums = 64): m_queue(maxTaskNums)
    {
        m_running = true;
        // 创建固定数量的线程
        for (int i = 0; i < threadNum; i++)
        {
            m_threads.emplace_back(std::make_shared<std::thread> (
                &ThreadPool::ThreadPoll,
                this
            ));
        }
    }

    ~ThreadPool()
    {
        std::call_once(m_flag, [this]() {
            Shutdown();
        })
    }

    void AddTask(Task &&task)
    {
        m_queue.Add(std::forward<Task> (task));
    }

    void AddTask(const Task &task)
    {
        m_queue.Add(task);
    }


private:
    void ThreadPoll()
    {
        while (m_running)
        {
            std::list<Task> list;
            m_queue.BatchGet(list);
            for (auto &task: list)
            {
                if (!m_running)
                {
                    return;
                }
                task();
            }
        }
    }
    void Shutdown()
    {
        m_queue.Shutdown();
        m_running = false;

        for (auto thread : m_threads)
        {
            if (thread)
            {
                thread->join();
            }
        }
        m_threads.clear();
    }

private:
    SyncQueue m_queue;
    std::atomic_bool m_running;
    std::once_flag m_flag;
    std::vector<std::shared_ptr<std::thread>> m_threads;
}

#endif
