/**
 * @file ThreadPool.hpp
 * @author RT Team
 * @date 2026-07-05
 * @brief Reusable thread pool for parallel rendering
 *
 * Maintains a fixed set of worker threads that pull tasks from a queue.
 * Tasks are void()->void callables. The pool can enqueue multiple tasks,
 * and waitAll() blocks until all are complete.
 *
 * @relationships
 * - Used by: Renderer (distributes pixel regions for parallel rendering)
 * - Eliminates: per-frame thread creation/destruction overhead
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool
{
public:
    using Task = std::function<void()>;

    /**
     * @brief Construct a thread pool with the given number of workers
     * @param numThreads Number of worker threads (0 = no workers, caller runs tasks)
     */
    explicit ThreadPool(unsigned int numThreads);

    /**
     * @brief Destructor — signals all workers to stop and joins them
     */
    ~ThreadPool();

    // Non-copyable, non-movable
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    /**
     * @brief Enqueue a task for execution by worker threads
     * @param task Callable (void->void) to execute
     */
    void enqueue(Task task);

    /**
     * @brief Wait for all enqueued tasks to complete
     *
     * Blocks the calling thread until every previously enqueued task
     * has been picked up and finished by a worker.
     */
    void waitAll();

    /**
     * @brief Get the number of worker threads
     * @return Thread count
     */
    [[nodiscard]] unsigned int getThreadCount() const;

private:
    std::vector<std::thread> workers; ///< Worker threads
    std::queue<Task> tasks;           ///< Task queue
    std::mutex mutex;                 ///< Protects tasks queue
    std::condition_variable cv;       ///< Notifies workers of new tasks / stop
    std::mutex doneMutex;             ///< Protects doneCv
    std::condition_variable doneCv;   ///< Notifies waitAll() when pendingTasks reaches 0
    std::atomic<int> pendingTasks;    ///< Number of tasks not yet completed
    bool stopRequested;               ///< Flag to shut down workers

    /**
     * @brief Main loop for each worker thread
     */
    void workerLoop();
};
