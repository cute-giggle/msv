// Author: cute-giggle@outlook.com

#ifndef RESPONSE_H
#define RESPONSE_H

#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <map>
#include <fcntl.h>

#include "buffer.h"

namespace msv {

class Response {
public:
    Response() = default;
    ~Response()
    {
        Munmap();
    }

    void Initialize(const std::string& resDir, const std::string& path, bool keepAlive, int code = 200)
    {
        resDir_ = resDir;
        path_ = path;
        code_ = code;
        keepAlive_ = keepAlive;
        Munmap();
    }

    void MakeResponse(Buffer& buffer)
    {
        if(stat((resDir_ + path_).data(), &mmFileStat_) < 0 || S_ISDIR(mmFileStat_.st_mode)) {
            code_ = 404;
        }
        else if(!(mmFileStat_.st_mode & S_IROTH)) {
            code_ = 403;
        }
        if (code_ != 404 && code_ != 403 && code_ != 400 && code_ != 200) {
            code_ = 404;
        }

        static const std::map<int, std::string> errorHtmlMapping = {{400, "400.html"}, {403, "403.html"}, {404, "404.html"}};
        if (errorHtmlMapping.count(code_)) {
            path_ = errorHtmlMapping.find(code_)->second;
            stat((resDir_ + path_).data(), &mmFileStat_);
        }

        std::string responsHead;

        static const std::map<int, std::string> codeStatusMapping = {{200, "OK"}, {400, "Bad Request"}, {403, "Forbidden"}, {404, "Not Found"}};
        responsHead += "HTTP/1.1 " + std::to_string(code_) + " " + codeStatusMapping.find(code_)->second + "\r\n";
        
        if (keepAlive_) {
            responsHead += "Connection: keep-alive\r\nkeep-alive: max=6, timeout=120\r\n";
        } else {
            responsHead += "Connection: close\r\n";
        }

        static const std::map<std::string, std::string> contentTypeMapping = {
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
        auto ext = std::filesystem::path(path_).extension().string();
        if (!contentTypeMapping.count(ext)) {
            responsHead += "Content-type: text/plain\r\n";
        } else {
            responsHead += "Content-type: " + contentTypeMapping.find(ext)->second + "\r\n";
        }

        auto resFd = open((resDir_ + path_).c_str(), O_RDONLY);
        if (resFd < 0) {
            return;
        }

        int* mmRet = (int*)mmap(0, mmFileStat_.st_size, PROT_READ, MAP_PRIVATE, resFd, 0);
        if(*mmRet == -1) {
            return; 
        }

        mmFile_ = reinterpret_cast<char*>(mmRet);
        
    }

private:
    void Munmap()
    {
        if (mmFile_) {
            munmap(mmFile_, mmFileStat_.st_size);
            mmFile_ = nullptr;
            mmFileStat_ = {0};
        }
    }

private:
    int code_;
    bool keepAlive_;

    std::string path_;
    std::string resDir_;

    char* mmFile_; 
    struct stat mmFileStat_;
};

}

#endif