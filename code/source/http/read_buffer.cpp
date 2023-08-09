// Author: cute-giggle@outlook.com

#include <algorithm>

#include "http/read_buffer.h"

namespace msv::http {

bool ReadBuffer::ReadLT(int fd)
{
    if (fd < 0) {
        return false;
    }

    char buffer[MAX_ONCE_READ_SIZE] = {0};
    auto len = read(fd, buffer, sizeof(buffer));

    // read ready but no reads
    if (len <= 0) {
        return false;
    }

    std::copy(buffer, buffer + len, std::back_inserter(data_));
    return true;
}

bool ReadBuffer::ReadET(int fd)
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

std::optional<std::string> ReadBuffer::GetLine()
{
    static const char *CRLF = "\r\n";
    auto iter = std::search(data_.begin(), data_.end(), CRLF, CRLF + 2);
    if (iter == data_.end()) {
        return std::nullopt;
    }
    std::string line(data_.begin(), iter);
    data_.erase(data_.begin(), iter + 2);
    return std::make_optional(line);
}

std::optional<std::string> ReadBuffer::GetBytes(std::size_t n)
{
    n = std::min(Size(), n);
    std::string data(data_.begin(), data_.begin() + n);
    data_.erase(data_.begin(), data_.begin() + n);
    return std::make_optional(data);
}


}