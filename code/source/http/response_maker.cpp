// Author: cute-giggle@outlook.com

#include "http/response_maker.h"

namespace msv::http {

ResponseData ResponseMaker::Make(const Path& resPath, int code, bool isKeepAlive)
{
    if (code != 200 && code != 400 && code != 404) {
        MLOG_WARNN("Response maker unsupported code: ", code);
        code = 400;
    }

    std::size_t resSize = 0;
    void* resMmPtr = nullptr;
    if (std::filesystem::exists(resPath) && !std::filesystem::is_directory(resPath)) {
        auto resFd = open(resPath.c_str(), O_RDONLY);
        if (resFd < 0) {
            MLOG_WARNN("Response maker open file failed! path: ", resPath.c_str());
            code = 404;
        }
        Reset();
        resSize = std::filesystem::file_size(resPath);
        resMmPtr = mmap(0, resSize, PROT_READ, MAP_PRIVATE, resFd, 0);
        if (*reinterpret_cast<int*>(resMmPtr) == -1) {
            MLOG_WARNN("Response maker mmap file failed! path: ", resPath.c_str());
            code = 404;
        }
        mmapInfo_ = {resMmPtr, resSize};
        close(resFd);
    } else {
        MLOG_WARNN("Resource file not exist or is a directory! path: ", resPath.c_str());
        code = 404;
    }

    std::string header;
    header += "HTTP/1.1 " + std::to_string(code) + " " + GetCodeStatus(code) + "\r\n";
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
        resSize = errorBody.length();
        header += "Content-length: " + std::to_string(resSize) + "\r\n\r\n";
        body = errorBody.c_str();
    }

    return {header, body, resSize};
}

std::string ResponseMaker::GetContentType(const Path &resPath)
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

}