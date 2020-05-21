#ifndef SYNC_QUEUE_H
#define SYNC_QUEUE_H

#include <iostream>
#include <list>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <mutex>

template<typename T>
class SyncQueue
{
public:
    SyncQueue(int maxSize, int writeBuffSize = 16): m_writeBuffSize(writeBuffSize), m_maxSize(maxSize)
    {
        m_shutdown = false;
    }

    ~SyncQueue() {}

    template<typename F>
    void Add(F &&x)
    {
        std::unique_lock<std::mutex> locker(m_mutex);
        m_notFull.wait(locker, [this]() {
            return m_shutdown || m_queue.size() <= m_maxSize;
        });
        if (m_shutdown)
        {
            std::cout << "The queue can not add anything\n";
            return;
        }
        m_queue.emplace_back(std::forward<F>(x));
        m_notEmplty.notify_one();
    }


    bool TryAdd(std::function<void()> lambda)
    {
        std::unique_lock<std::mutex> locker(m_mutex, std::try_to_lock);
        if (locker.owns_lock())
        {
            lambda();
            return true;
        }
        else
        {
            return false;
        }
    }

    void Put(T &&x)
    {
        Add(std::forward<T> (x));
    }

    void Put(const T &x)
    {
        Add(x);
    }



    void BatchGet(std::list<T> &list)
    {
        std::unique_lock<std::mutex> locker(m_mutex);
        m_notEmpty.wait(locker, [this]() {
            return m_shutdown || !m_queue.empty();
        });
        if (m_shutdown)
        {
            std::cout << "The queue can not get anything\n";
            return;
        }
        list = std::move(m_queue);
        m_notFull.notify_one();
    }

    bool TryBatchGet(std::function<void()> lambda)
    {
        std::unique_lock<std::mutex> locker(m_mutex, std::try_to_lock);
        if (locker.owns_lock())
        {
            lambda();
            return true;
        }
        else
        {
            return false;
        }
    }

private:
    std::list<T> m_queue;
    std::list<std::list<T>> m_queue2;
    std::list<T> m_writeBuffer;
    int m_writeBuffSize;
    int m_maxSize;
    std::atomic_bool m_shutdown;
    std::mutex m_mutex;
    std::condition_variable m_notFull;
    std::condition_variable m_notEmpty;
};

#endif
