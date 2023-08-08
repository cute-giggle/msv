//  Author cute-giggle@outlook.com

#ifndef CONNECTION_H
#define CONNECTION_H

#include <arpa/inet.h>

#include "buffer.h"
#include "request.h"

namespace msv {

class Connection {
public:
    enum class TriggerMode: uint8_t {
        TM_LT = 0U,
        TM_ET,
    };

public:

    explicit Connection(int cfd, struct sockaddr_in caddr) : cfd_(cfd), caddr_(caddr) {}

    ~Connection()
    {
        Close();
    }

    void Close()
    {
        [[maybe_unused]] auto ret = close(cfd_);
        cfd_ = -1;
        caddr_ = {0, {0}, {0}};
    }

    bool Read()
    {
        if (triggerMode_ == TriggerMode::TM_LT) {
            return readBuffer_.ReadLT(cfd_);
        }
        return readBuffer_.ReadET(cfd_);
    }

    bool Write()
    {
        if (triggerMode_ == TriggerMode::TM_LT) {
            return writeBuffer_.WriteLT(cfd_);
        }
        return writeBuffer_.WriteET(cfd_);
    }

    bool Process()
    {
        auto code = request_.Parse(readBuffer_);
        if (code == Request::HttpCode::NO_REQUEST) {
            return false;
        }
        if (code == Request::HttpCode::GET_REQUEST) {

        }
        
    }

    int GetFd() const
    {
        return cfd_;
    }

    bool NeedToWrite() const
    {
        return !writeBuffer_.Empty();
    }

    bool IsKeepAlive() const
    {
        return request_.IsKeepAlive();
    }

private:
    TriggerMode triggerMode_ = TriggerMode::TM_LT;
    int cfd_ = -1;
    struct sockaddr_in caddr_ = {0, {0}, {0}};
    Buffer readBuffer_;
    Buffer writeBuffer_;
    Request request_;
};

}

#endif