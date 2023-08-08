//  Author cute-giggle@outlook.com

#ifndef CONNECTION_H
#define CONNECTION_H

#include <arpa/inet.h>

#include "read_buffer.h"
#include "request_parser.h"
#include "response_maker.h"

namespace msv {

namespace http {

enum class TriggerMode: uint8_t {
    TM_LT = 0U,
    TM_ET,
};

class HttpConnection {
public:
    explicit HttpConnection() = default;

    ~HttpConnection()
    {
        Close();
    }

    void Initialize(TriggerMode triggerMode, int cfd, struct sockaddr_in caddr)
    {
        triggerMode_ = triggerMode;
        cfd_ = cfd;
        caddr_ = caddr;
        closed_ = false;
        responseData_ = {};
        bzero(writeBuffer_, sizeof(struct iovec) * 2);
        readBuffer_.Clear();
        requestParser_.Reset();
    }

    void Close()
    {
        [[maybe_unused]] auto ret = close(cfd_);
        Initialize(TriggerMode::TM_LT, -1, {});
        closed_ = true;
    }

    bool Read()
    {
        if (triggerMode_ == TriggerMode::TM_LT) {
            return readBuffer_.ReadLT(cfd_);
        }
        return readBuffer_.ReadET(cfd_);
    }

    bool Process()
    {
        auto parseRet = requestParser_.Parse(readBuffer_);
        using RetStatus = RequestParser::RetStatus;
        if (parseRet == RetStatus::NO_REQUEST) {
            return false;
        }
        if (parseRet == RetStatus::BAD_REQUEST) {
            auto resPath = std::filesystem::path(ResDir).append("400.html");
            responseData_ = responseMaker_.Make(resPath, 400, false);
        } else {
            auto resPath = std::filesystem::path(ResDir).append(requestParser_.GetPath());
            responseData_ = responseMaker_.Make(resPath, 200, requestParser_.IsKeepAlive());
        }
        writeBuffer_[0].iov_base = const_cast<char*>(responseData_.header.c_str());
        writeBuffer_[0].iov_len = responseData_.header.length();
        if (responseData_.body != nullptr) {
            writeBuffer_[1].iov_base = const_cast<char*>(responseData_.body);
            writeBuffer_[1].iov_len = responseData_.bodyLength;
        }
        return true;
    }

    bool Write()
    {
        auto iovCnt = writeBuffer_[1].iov_base ? 2 : 1;
        while (writeBuffer_[0].iov_len + writeBuffer_[1].iov_len) {
            auto len = writev(cfd_, writeBuffer_, iovCnt);
            if (len < 0) {
                return errno == EAGAIN;
            }
            if (len == 0) {
                return false;
            }

            if (len >= writeBuffer_[0].iov_len) {
                writeBuffer_[1].iov_base = reinterpret_cast<uint8_t*>(writeBuffer_[1].iov_base) + len - writeBuffer_[0].iov_len;
                writeBuffer_[1].iov_len -= len - writeBuffer_[0].iov_len;
                writeBuffer_[0].iov_base = nullptr;
                writeBuffer_[0].iov_len = 0;
            } else {
                writeBuffer_[0].iov_base =  reinterpret_cast<uint8_t*>(writeBuffer_[0].iov_base) + len;
                writeBuffer_[0].iov_len -= len;
            }
        }
        return true;
    }

    bool WriteComplete() const
    {
        return writeBuffer_[0].iov_len + writeBuffer_[1].iov_len != 0;
    }

    int GetFd() const
    {
        return cfd_;
    }

    bool IsKeepAlive() const
    {
        return requestParser_.IsKeepAlive();
    }

    static std::filesystem::path ResDir;

private:
    TriggerMode triggerMode_ = TriggerMode::TM_LT;
    int cfd_ = -1;
    struct sockaddr_in caddr_ = {0, {0}, {0}};

    bool closed_{};
    ResponseData responseData_;
    struct iovec writeBuffer_[2]{};

    ReadBuffer readBuffer_;
    RequestParser requestParser_;
    ResponseMaker responseMaker_;
};

}

}

#endif