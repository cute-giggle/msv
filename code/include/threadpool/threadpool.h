// Author: cute-giggle@outlook.com

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <mutex>
#include <queue>
#include <functional>
#include <thread>
#include <condition_variable>

namespace msv {

class TaskQueue {
public:
    using TaskType = std::function<void()>;

    bool shutdown_{};
    std::mutex mutex_{};
    std::condition_variable cond_{};
    std::queue<TaskType> queue_{};

    void Shutdown()
    {
        {
            std::lock_guard locker(mutex_);
            shutdown_ = true;
        }
        cond_.notify_all();
    }

    void AddTask(TaskType&& task)
    {
        {
            std::lock_guard locker(mutex_);
            queue_.push(std::move(task));
        }
        cond_.notify_one();
    }
};

class ThreadPool {
public:
    static constexpr uint32_t MAX_THREAD_COUNT = 16U;

public:
    ThreadPool () = default;

    // ThreadPool(const ThreadPool& rhs) = delete;
    // ThreadPool& operator=(const ThreadPool& rhs) = delete;

    ~ThreadPool()
    {
        if (taskQueue_) {
            taskQueue_->Shutdown();
        }
    }

    void Shutdown()
    {
        taskQueue_->Shutdown();
    }

    void AddTask(TaskQueue::TaskType&& task)
    {
        taskQueue_->AddTask(std::move(task));
    }

    void Initialize(uint32_t threadCount)
    {
        threadCount_ = std::min(threadCount, MAX_THREAD_COUNT);
        taskQueue_ = std::make_shared<TaskQueue>();

        for (uint32_t i = 0U; i < threadCount_; ++i) {
            auto threadFunc = [tasks=taskQueue_]() {
                auto locker = std::unique_lock(tasks->mutex_);
                while (true) {
                    if (tasks->queue_.empty()) {
                        tasks->cond_.wait(locker);
                    }
                    if (!tasks->queue_.empty()) {
                        auto task = std::move(tasks->queue_.front());
                        tasks->queue_.pop();
                        locker.unlock();
                        task();
                        locker.lock();
                    }
                    if (tasks->shutdown_) {
                        break;
                    }
                    
                }
            };
            std::thread(std::move(threadFunc)).detach();
        }
    }

private:
    uint32_t threadCount_;
    std::shared_ptr<TaskQueue> taskQueue_;
};

}

#endif