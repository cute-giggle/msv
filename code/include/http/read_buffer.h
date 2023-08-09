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

    bool ReadLT(int fd)
    {
        char buffer[MAX_ONCE_READ_SIZE] = {0};
        auto len = read(fd, buffer, sizeof(buffer));
        
        // read ready but no reads
        if (len <= 0) {
            return false;
        }

        std::copy(buffer, buffer + len, std::back_inserter(data_));
        return true;
    }

    bool ReadET(int fd)
    {
        while (true) {
            char buffer[MAX_ONCE_READ_SIZE] = {0};
            auto len = read(fd, buffer, sizeof(buffer));

            if (len == -1) {
                // read complete
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                }
                return false;
            }

            // read ready but no reads
            if (len == 0) {
                return false;
            }

            std::copy(buffer, buffer + len, std::back_inserter(data_));
        }
        return true;
    }

    std::optional<std::string> GetLine()
    {
        static const char* CRLF = "\r\n";
        auto iter = std::search(data_.begin(), data_.end(), CRLF, CRLF + 2);
        if (iter == data_.end()) {
            return std::nullopt;
        }
        std::string line(data_.begin(), iter);
        data_.erase(data_.begin(), iter + 2);
        return std::make_optional(line);
    }

    std::optional<std::string> GetBytes(std::size_t n) {
        n = std::min(Size(), n);
        std::string data(data_.begin(), data_.begin() + n);
        data_.erase(data_.begin(), data_.begin() + n);
        return std::make_optional(data);
    }

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