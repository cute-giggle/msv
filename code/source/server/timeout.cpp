// Author: cute-giggle@outlook.com

#include "server/timeout.h"

namespace msv::timeout {

void TimeNodeHeap::Insert(int cfd, TimeStamp timeout, TimeoutCallbackFuncType&& callback)
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

void TimeNodeHeap::Modify(int cfd, TimeStamp timeout)
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

void TimeNodeHeap::Remove(int cfd)
{
    auto iter = userMapping_.find(cfd);
    if (iter == userMapping_.end()) {
        return;
    }
    auto index = iter->second;
    data_[index].callback();
    Remove(index);
}

TimeStamp TimeNodeHeap::GetMinTimeout()
{
    Clean();
    if (Empty()) {
        return -1;
    }
    auto ret = Top().expire - Now();
    return ret > 0 ? ret : -1;
}

bool TimeNodeHeap::AdjustDown(std::size_t index)
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

void TimeNodeHeap::AdjustUp(std::size_t index)
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

void TimeNodeHeap::Remove(std::size_t index)
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

void TimeNodeHeap::Clean()
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
}