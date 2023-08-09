// Author: cute-giggle@outlook.com

#include "http/http_connection.h"

namespace msv::http {

std::filesystem::path HttpConnection::ResDir = "";
std::atomic<uint32_t> HttpConnection::NumOnline{};

void HttpConnection::Initialize(TriggerMode triggerMode, int cfd, struct sockaddr_in caddr)
{
    triggerMode_ = triggerMode;
    cfd_ = cfd;
    caddr_ = caddr;
    closed_ = false;
    keepAlive_ = false;
    responseData_ = {};
    bzero(writeBuffer_, sizeof(struct iovec) * 2);
    readBuffer_.Clear();
    requestParser_.Reset();

    NumOnline += 1;
}

bool HttpConnection::Process()
{
    MLOG_DEBUG("Current request:\n", readBuffer_.String());

    auto parseRet = requestParser_.Parse(readBuffer_);
    using RetStatus = RequestParser::RetStatus;
    if (parseRet == RetStatus::NO_REQUEST) {
        return false;
    }
    if (parseRet == RetStatus::BAD_REQUEST) {
        auto resPath = std::filesystem::path(ResDir).append("400.html");
        responseData_ = responseMaker_.Make(resPath, 400, false);
    }
    else {
        auto resPath = ResDir.string() + requestParser_.GetPath();
        responseData_ = responseMaker_.Make(resPath, 200, requestParser_.IsKeepAlive());
    }

    MLOG_DEBUG("Response header:\n", responseData_.header);

    writeBuffer_[0].iov_base = const_cast<char *>(responseData_.header.c_str());
    writeBuffer_[0].iov_len = responseData_.header.length();
    if (responseData_.body != nullptr) {
        writeBuffer_[1].iov_base = const_cast<char *>(responseData_.body);
        writeBuffer_[1].iov_len = responseData_.bodyLength;
    }
    keepAlive_ = requestParser_.IsKeepAlive();
    requestParser_.Reset();
    return true;
}

bool HttpConnection::Write()
{
    auto iovCnt = writeBuffer_[1].iov_base ? 2 : 1;
    while (!WriteComplete()) {
        auto len = writev(cfd_, writeBuffer_, iovCnt);
        if (len < 0) {
            return errno == EAGAIN;
        }
        if (len == 0) {
            return false;
        }

        if (len >= writeBuffer_[0].iov_len) {
            writeBuffer_[1].iov_base = reinterpret_cast<uint8_t *>(writeBuffer_[1].iov_base) + len - writeBuffer_[0].iov_len;
            writeBuffer_[1].iov_len -= len - writeBuffer_[0].iov_len;
            writeBuffer_[0].iov_base = nullptr;
            writeBuffer_[0].iov_len = 0;
        }
        else {
            writeBuffer_[0].iov_base = reinterpret_cast<uint8_t *>(writeBuffer_[0].iov_base) + len;
            writeBuffer_[0].iov_len -= len;
        }
    }
    return true;
}
}