// Author: cute-giggle@outlook.com

#include "threadpool/threadpool.h"

namespace msv {

void ThreadPool::Initialize(uint32_t threadCount)
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

}