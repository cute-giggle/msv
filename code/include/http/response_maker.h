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

class ResponseMaker {
public:

    static ResponseData Make(const Path& resPath, int code, bool isKeepAlive)
    {
        auto resFd = open(resPath.c_str(), O_RDONLY);
        if (resFd < 0) {
            MLOG_WARNN("Response maker open file failed! path: ", resPath.c_str());
            code = 404;
        }

        std::size_t resSize = 0;
        void* resMmPtr = nullptr;
        if (resFd > 0) {
            resSize = std::filesystem::file_size(resPath);
            resMmPtr = mmap(0, resSize, PROT_READ, MAP_PRIVATE, resFd, 0);
        }
        
        if (resMmPtr == nullptr || *reinterpret_cast<int*>(resMmPtr) == -1) {
            MLOG_WARNN("Response maker mmap file failed! path: ", resPath.c_str());
            code = 404;
        }

        if (code != 200 && code != 400 && code != 404) {
            MLOG_WARNN("Response maker unsupported code: ", code);
            code = 400;
        }

        std::string header;
        if (isKeepAlive) {
            header += "Connection: keep-alive\r\nkeep-alive: max=6, timeout=120\r\n";
        } else {
            header += "Connection: close\r\n";
        }
        if (code == 200) {
            header += GetContentType(resPath);
        } else {
            header += "Content-type: text/html\r\n";
        }

        const char* body = nullptr;
        if (code == 200) {
            header += "Content-length: " + std::to_string(resSize) + "\r\n\r\n";
            body = reinterpret_cast<const char*>(resMmPtr);
        } else {
            const auto& errorBody = GetErrorBody(code);
            header += "Content-length: " + std::to_string(errorBody.length()) + "\r\n\r\n";
            body = errorBody.c_str();
        }

        return {header, body, resSize};
    }

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

    static std::string GetContentType(const Path& resPath)
    {
        static const std::map<std::string, std::string> mapping = {
            {".html", "text/html"},
            {".xml", "text/xml"},
            {".xhtml", "application/xhtml+xml"},
            {".txt", "text/plain"},
            {".rtf", "application/rtf"},
            {".pdf", "application/pdf"},
            {".word", "application/nsword"},
            {".png", "image/png"},
            {".gif", "image/gif"},
            {".jpg", "image/jpeg"},
            {".jpeg", "image/jpeg"},
            {".au", "audio/basic"},
            {".mpeg", "video/mpeg"},
            {".mpg", "video/mpeg"},
            {".avi", "video/x-msvideo"},
            {".gz", "application/x-gzip"},
            {".tar", "application/x-tar"},
            {".css", "text/css"},
            {".js", "text/javascript"},
        };
        auto ext = resPath.extension().string();
        auto iter = mapping.find(ext);
        if (iter == mapping.end()) {
            return "Content-type: text/plain\r\n";
        }
        return "Content-type: " + iter->second + "\r\n";
    }
};

}

}

#endif