// Author: cute-giggle@outlook.com

#ifndef TIMEOUT_H
#define TIMEOUT_H

#include <chrono>
#include <functional>
#include <unordered_map>
#include <cassert>

#include "utils/mlog.h"

namespace msv {

namespace timeout {

using TimeStamp = int64_t;
using TimeoutCallbackFuncType = std::function<void()>;

struct TimeNode {
    int cfd;
    TimeStamp expire;
    TimeoutCallbackFuncType callback;

    bool operator<(const TimeNode& rhs) const
    {
        return expire < rhs.expire;
    }
};

class TimeNodeHeap {
public:
    TimeNodeHeap() = default;
    ~TimeNodeHeap() = default;

    void Insert(int cfd, TimeStamp timeout, TimeoutCallbackFuncType&& callback);
    void Modify(int cfd, TimeStamp timeout);
    void Remove(int cfd);

    TimeStamp GetMinTimeout();

private:
    bool AdjustDown(std::size_t index);
    void AdjustUp(std::size_t index);
    void Remove(std::size_t index);

    const TimeNode& Top() const
    {
        return data_[0];
    }

    void Pop()
    {
        Remove(0UL);
    }

    bool Empty() const
    {
        return data_.empty();
    }

    void Clean();

    static TimeStamp Now()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }

private:
    std::vector<TimeNode> data_;
    std::unordered_map<int, std::size_t> userMapping_;
};

}

using timeout::TimeNodeHeap;
using timeout::TimeoutCallbackFuncType;
using timeout::TimeStamp;

}

#endif