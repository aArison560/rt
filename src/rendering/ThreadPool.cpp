/**
 * @file ThreadPool.cpp
 * @author RT Team
 * @date 2026-07-05
 * @brief Thread pool implementation
 */

#include "rendering/ThreadPool.hpp"
#include <utility>

ThreadPool::ThreadPool(unsigned int numThreads)
    : pendingTasks(0)
    , stopRequested(false)
{
    workers.reserve(numThreads);
    for (unsigned int i = 0; i < numThreads; ++i) {
        workers.emplace_back(&ThreadPool::workerLoop, this);
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::lock_guard<std::mutex> lock(mutex);
        stopRequested = true;
    }
    cv.notify_all();

    for (auto& t : workers) {
        if (t.joinable()) {
            t.join();
        }
    }
}

void ThreadPool::enqueue(Task task)
{
    {
        std::lock_guard<std::mutex> lock(mutex);
        tasks.push(std::move(task));
    }
    pendingTasks.fetch_add(1, std::memory_order_release);
    cv.notify_one();
}

void ThreadPool::waitAll()
{
    std::unique_lock<std::mutex> lock(doneMutex);
    doneCv.wait(lock, [this]() {
        return pendingTasks.load(std::memory_order_acquire) == 0;
    });
}

unsigned int ThreadPool::getThreadCount() const
{
    return static_cast<unsigned int>(workers.size());
}

void ThreadPool::workerLoop()
{
    while (true) {
        Task task;
        {
            std::unique_lock<std::mutex> lock(mutex);
            cv.wait(lock, [this]() {
                return stopRequested || !tasks.empty();
            });

            if (stopRequested && tasks.empty()) {
                return;
            }

            task = std::move(tasks.front());
            tasks.pop();
        }

        // Execute the task outside the lock
        if (task) {
            task();
        }

        pendingTasks.fetch_sub(1, std::memory_order_release);
        {
            std::lock_guard<std::mutex> lock(doneMutex);
        }
        doneCv.notify_one();
    }
}
