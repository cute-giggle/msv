// Author: cute-giggle@outlook.com

#ifndef TIMEOUT_H
#define TIMEOUT_H

#include <chrono>
#include <functional>
#include <unordered_map>
#include <cassert>

#include <iostream>

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

    void Insert(int cfd, TimeStamp timeout, TimeoutCallbackFuncType&& callback)
    {
        auto iter = userMapping_.find(cfd);
        if (iter != userMapping_.end()) {
            data_[iter->second] = {cfd, Now() + timeout, std::move(callback)};
            if (!AdjustDown(iter->second)) {
                AdjustUp(iter->second);
            }
            MLOG_DEBUG("Time node heap: node reuse.");
            return;
        }
        userMapping_.emplace(cfd, data_.size());
        data_.push_back({cfd, Now() + timeout, std::move(callback)});
        AdjustUp(data_.size() - 1);
    }

    void Modify(int cfd, TimeStamp timeout)
    {
        if (cfd < 0) {
            MLOG_DEBUG("Modify time node heap failed! invalid fd: ", cfd);
            return;
        }
        auto iter = userMapping_.find(cfd);
        if (iter == userMapping_.end()) {
            MLOG_DEBUG("Modify time node heap failed! fd not exist. fd: ", cfd);
            return;
        }
        data_[iter->second].expire = Now() + timeout;
        if (!AdjustDown(iter->second)) {
            AdjustUp(iter->second);
        }
    }

    void Remove(int cfd)
    {
        auto iter = userMapping_.find(cfd);
        if (iter == userMapping_.end()) {
            return;
        }
        auto index = iter->second;
        data_[index].callback();
        Remove(index);
    }

    TimeStamp GetMinTimeout()
    {
        Clean();
        if (Empty()) {
            return -1;
        }
        auto ret = Top().expire - Now();
        return ret > 0 ? ret : -1;
    }

private:
    bool AdjustDown(std::size_t index)
    {
        auto backup = index;
        auto size = data_.size();
        auto child = index * 2 + 1;
        while (child < size) {
            if (child + 1 < size && data_[child + 1] < data_[child]) {
                child++;
            }
            if (!(data_[child] < data_[index])) {
                break;
            }
            std::swap(data_[child], data_[index]);
            std::swap(userMapping_[data_[child].cfd], userMapping_[data_[index].cfd]);
            index = child;
            child = index * 2 + 1;
        }
        return backup != index;
    }

    void AdjustUp(std::size_t index)
    {
        auto size = data_.size();
        auto parent = (index - 1) / 2;
        while (parent > 0 && parent < size) {
            if (!(data_[index] < data_[parent])) {
                break;
            }
            std::swap(data_[parent], data_[index]);
            std::swap(userMapping_[data_[parent].cfd], userMapping_[data_[index].cfd]);
            index = parent;
            parent = (parent - 1) / 2;
        }
    }

    void Remove(std::size_t index)
    {
        auto last = data_.size() - 1;
        auto cfd = data_[index].cfd;
        if (index != last) {
            std::swap(data_[last], data_[index]);
            std::swap(userMapping_[data_[last].cfd], userMapping_[data_[index].cfd]);
        }
        data_.pop_back();
        userMapping_.erase(cfd);
        if (index != last && !AdjustDown(index)) {
            AdjustUp(index);
        }
    }

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

    void Clean()
    {
        auto now = Now();
        while (!Empty()) {
            const auto& node = Top();
            if (node.expire > now) {
                break;
            }
            node.callback();
            Pop();
        }
    }

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