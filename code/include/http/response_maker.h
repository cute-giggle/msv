// Author: cute-giggle@outlook.com

#ifndef RESPONSE_MAKER_H
#define RESPONSE_MAKER_H

#include <string>
#include <filesystem>
#include <unistd.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <map>

#include "utils/mlog.h"

namespace msv {

namespace http {

using Path = std::filesystem::path;

struct ResponseData {
    std::string header{};
    const char* body{};
    std::size_t bodyLength{};
};

struct MmapInfo {
    void* ptr{};
    std::size_t size{};
};

class ResponseMaker {
public:
    ResponseMaker() = default;

    ~ResponseMaker()
    {
        Reset();
    }

    void Reset()
    {
        if (mmapInfo_.ptr) {
            munmap(mmapInfo_.ptr, mmapInfo_.size);
        }
        mmapInfo_ = {};
    }

    ResponseData Make(const Path& resPath, int code, bool isKeepAlive);

private:
    static const std::string& GetErrorBody(int code)
    {
        static const std::string html400 = "<html><title>Error</title><body><p>400 Bad Request!</p></body></html>";
        static const std::string html404 = "<html><title>Error</title><body><p>404 Not found!</p></body></html>";

        return code == 400 ? html400 : html404;
    }

    static std::string GetCodeStatus(int code)
    {
        static const std::map<int, std::string> mapping = {{200, "OK"}, {400, "Bad Request"}, {404, "Not Found"}};
        return mapping.find(code)->second;
    }

    static std::string GetResponseLine(int code)
    {
        return "HTTP/1.1 " + std::to_string(code) + " " + GetCodeStatus(code) + "\r\n";
    }

    static std::string GetContentType(const Path& resPath);

private:
    MmapInfo mmapInfo_;
};

}

}

#endif