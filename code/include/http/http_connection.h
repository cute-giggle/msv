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
    HttpConnection() = default;

    ~HttpConnection()
    {
        Close();
    }

    void Initialize(TriggerMode triggerMode, int cfd, struct sockaddr_in caddr);

    void Close()
    {
        [[maybe_unused]] auto ret = close(cfd_);
        Initialize(TriggerMode::TM_LT, -1, {});
        closed_ = true;

        NumOnline -= 1;
    }

    bool IsClosed() const
    {
        return closed_ == true;
    }

    bool Read()
    {
        if (triggerMode_ == TriggerMode::TM_LT) {
            return readBuffer_.ReadLT(cfd_);
        }
        return readBuffer_.ReadET(cfd_);
    }

    bool Process();

    bool Write();

    bool WriteComplete() const
    {
        return (writeBuffer_[0].iov_len + writeBuffer_[1].iov_len) == 0;
    }

    int GetFd() const
    {
        return cfd_;
    }

    bool IsKeepAlive() const
    {
        return keepAlive_;
    }

    static std::filesystem::path ResDir;
    static std::atomic<uint32_t> NumOnline;

private:
    TriggerMode triggerMode_ = TriggerMode::TM_LT;
    int cfd_ = -1;
    struct sockaddr_in caddr_ = {0, {0}, {0}};

    bool closed_{};
    bool keepAlive_{};
    ResponseData responseData_{};
    struct iovec writeBuffer_[2]{};

    ReadBuffer readBuffer_{};
    RequestParser requestParser_{};
    ResponseMaker responseMaker_{};
};

}

}

#endif