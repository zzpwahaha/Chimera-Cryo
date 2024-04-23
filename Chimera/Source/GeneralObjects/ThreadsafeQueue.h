#pragma once
#include <condition_variable> 
#include <mutex> 
#include <queue> 

// Thread-safe queue 
template <typename T>
class ThreadsafeQueue {
public:
    void push(T item);
    T pop();
    void clear();
    bool empty() const;

private:
    std::queue<T> m_queue;
    // mutex for thread synchronization 
    std::mutex m_mutex;
    // Condition variable for signaling 
    std::condition_variable m_cv;

};

template<typename T>
void ThreadsafeQueue<T>::push(T item)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_queue.push(item);
    m_cv.notify_one(); // Notify one thread that is waiting 
}

template<typename T>
T ThreadsafeQueue<T>::pop()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    // if queue is empty, wait until queue is not empty, notified by push()
    m_cv.wait(lock,
        [this]() { return !m_queue.empty(); });
    T item = m_queue.front();
    m_queue.pop();
    return item;
}

template<typename T>
void ThreadsafeQueue<T>::clear()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    std::queue<T> newQueue;
    std::swap(m_queue, newQueue);
}

template<typename T>
bool ThreadsafeQueue<T>::empty() const
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_queue.empty();
}
