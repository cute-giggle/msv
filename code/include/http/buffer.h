// Author: cute-giggle@outlook.com

#ifndef BUFFER_H
#define BUFFER_H

#include <cstdint>
#include <vector>
#include <cassert>
#include <unistd.h>

#include "utils/mlog.h"

namespace msv {

class Buffer {
public:
    explicit Buffer(uint32_t initSize = 1024U) : readPos_(0U), writePos_(0U), data(initSize, 0) {}
    ~Buffer() = default;

    static constexpr uint32_t MAX_ONCE_READ_SIZE = 4096U;

    void Clear()
    {
        readPos_ = 0U;
        writePos_ = 0U;
    }

    bool Empty() const
    {
        return readPos_ == writePos_;
    }

    void Expand(uint32_t need)
    {
        if (readPos_ > (data.size() >> 1))
        {
            std::vector<char> temp(data.begin() + readPos_, data.end());
            data = std::move(temp);
            writePos_ -= readPos_;
            readPos_ = 0U;
        }
        while (data.size() - writePos_ < need)
        {
            data.resize(data.size() << 1, 0);
        }
    }

    void MoveReadPos(uint32_t len)
    {
        assert(readPos_ + len <= writePos_);
        readPos_ += len;
    }

    bool ReadLT(int fd)
    {
        char buffer[MAX_ONCE_READ_SIZE] = {0};
        auto len = read(fd, buffer, sizeof(buffer));

        // read ready but no reads
        if (len <= 0) {
            return false;
        }

        if (data.size() - writePos_ < static_cast<uint32_t>(len)) {
            Expand(len);
        }
        std::copy(buffer, buffer + len, data.data() + writePos_);
        writePos_ += len;

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

            if (data.size() - writePos_ < static_cast<uint32_t>(len))
            {
                Expand(len);
            }
            std::copy(buffer, buffer + len, data.data() + writePos_);
            writePos_ += len;
        }
        return true;
    }

    bool WriteLT(int fd)
    {
        // write complete
        if (writePos_ - readPos_ == 0) {
            return true;
        }

        auto len = write(fd, data.data() + readPos_, writePos_ - readPos_);

        // write ready but no writes
        if (len <= 0) {
            return false;
        }

        readPos_ += len;
        return true;
    }

    bool WriteET(int fd)
    {
        while (true) {
            // write complete
            if (writePos_ - readPos_ == 0) {
                break;
            }

            auto len = write(fd, data.data() + readPos_, writePos_ - readPos_);

            if (len == -1) {
                // write full
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                }
            }

            // write ready but no writes
            if (len == 0) {
                return false;
            }

            readPos_ += len;
        }
        return true;
    }

    std::string Str()
    {
        return std::string(data.data() + readPos_, data.data() + writePos_);
    }

    std::string_view View()
    {
        return std::string_view(data.data() + readPos_, writePos_ - readPos_);
    }

private:
    uint32_t readPos_;
    uint32_t writePos_;
    std::vector<char> data;
};

}

#endif