// Author: cute-giggle@outlook.com

#ifndef READ_BUFFER_H
#define READ_BUFFER_H

#include <deque>
#include <errno.h>
#include <unistd.h>
#include <string>
#include <optional>

namespace msv {

namespace http {

class ReadBuffer {
public:
    static constexpr std::size_t MAX_ONCE_READ_SIZE = 4096U;

    bool ReadLT(int fd);
    bool ReadET(int fd);

    std::optional<std::string> GetLine();
    std::optional<std::string> GetBytes(std::size_t n);

    bool Empty() const
    {
        return data_.empty();
    }

    std::size_t Size() const
    {
        return data_.size();
    }

    void RemoveFront(std::size_t n)
    {
        n = std::min(n, Size());
        data_.erase(data_.begin(), data_.begin() + n);
    }

    void Clear()
    {
        data_.clear();
    }

    std::string String() const
    {
        return std::string(data_.begin(), data_.end());
    }

private:
    std::deque<char> data_;
};

}

}

#endif