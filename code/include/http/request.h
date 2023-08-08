// Author: cute-giggle@outlook.com

#ifndef REQUEST_H
#define REQUEST_H

#include <cstdint>
#include <vector>
#include <regex>
#include <map>
#include <set>
#include <sstream>

#include "buffer.h"

namespace msv {

class Request {
public:
    enum class ParseStatus: uint8_t {
        PS_REQUESTLINE = 0U,
        PS_HEADER,
        PS_BODY,
        PS_FINISH,
    };

    enum class LineStatus: uint8_t {
        LS_OPEN = 0U,
        LS_OK,
        LS_BAD,
    };

    enum class HttpCode: uint8_t {
        NO_REQUEST = 0U,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURSE,
        FORBIDDENT_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION,
    };

public:
    Request() = default;
    ~Request() = default;

    // Get a complete line
    std::pair<std::string, LineStatus> ParseLine(Buffer& rdbuf)
    {
        auto view = rdbuf.View();
        auto posr = view.find_first_of('\r');
        auto posn = view.find_first_of('\n');
        if (posr == view.npos && posn == view.npos) {
            return {"", LineStatus::LS_OPEN};
        }
        if (posr == view.npos) {
            return {"", LineStatus::LS_BAD};
        }
        if (posn == view.npos) {
            return {"", posr == view.length() - 1 ? LineStatus::LS_OPEN : LineStatus::LS_BAD};
        }
        if (posr + 1 != posn) {
            return {"", LineStatus::LS_BAD};
        }
        rdbuf.MoveReadPos(posn + 1);
        return {std::string(view.substr(0, posr)), LineStatus::LS_OK};
    }

    // Parse request line, only support GET POST HTTP1.1
    bool ParseRequestLine(const std::string& line)
    {
        static const std::set<std::string> SUPPORTED_METHODS = {"POST", "GET"};
        static const std::set<std::string> SUPPORTED_VERSION = {"1.1"};

        std::regex expr("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
        std::smatch result;
        if (std::regex_match(line, result, expr)) {
            method_ = std::move(result[0]);
            path_ = std::move(result[1]);
            version_ = std::move(result[2]);
            return SUPPORTED_METHODS.count(method_) && SUPPORTED_VERSION.count(version_) && CheckPath();
        }
        return false;
    }

    // Parse one header field
    bool ParseHeader(const std::string& line)
    {
        std::regex expr("^([^:]*): ?(.*)$");
        std::smatch result;
        if (std::regex_match(line, result, expr)) {
            header_[result[0]] = result[1];
            return true;
        }
        return false;
    }

    // Parse simple POST body
    bool ParseBody(const std::string& line)
    {
        if (method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") {
            auto str = line;
            std::replace(str.begin(), str.end(), '&', ' ');
            std::replace(str.begin(), str.end(), '=', ' ');
            std::stringstream ss(str);
            while (!ss.eof()) {
                std::string key, value;
                ss >> key >> value;
                std::replace(key.begin(), key.end(), '+', ' ');
                std::replace(value.begin(), value.end(), '+', ' ');
                post_[key] = value;
            }
            return true;
        }
        return false;
    }

    HttpCode Parse(Buffer& rdbuf)
    {
        while(true) {
            auto [line, status] = ParseLine(rdbuf);
            if (status == LineStatus::LS_BAD) {
                return HttpCode::BAD_REQUEST;
            }
            if (status == LineStatus::LS_OPEN) {
                return HttpCode::NO_REQUEST;
            }
            switch (parseStatus_)
            {
            case ParseStatus::PS_REQUESTLINE:
                if (!ParseRequestLine(line)) {
                    return HttpCode::BAD_REQUEST;
                }
                parseStatus_ = ParseStatus::PS_HEADER;
                break;
            case ParseStatus::PS_HEADER:
                if (!ParseHeader(line)) {
                    if (!line.empty()) {
                        return HttpCode::BAD_REQUEST;
                    }
                    if (method_ != "POST") {
                        parseStatus_ = ParseStatus::PS_FINISH;
                        return HttpCode::GET_REQUEST;
                    }
                    parseStatus_ = ParseStatus::PS_BODY;
                }
                break;
            case ParseStatus::PS_BODY:
                if (!ParseBody(line)) {
                    return HttpCode::BAD_REQUEST;
                }
                parseStatus_ = ParseStatus::PS_FINISH;
                return HttpCode::GET_REQUEST;
                break;
            default:
                return HttpCode::INTERNAL_ERROR;
            }
        }
        return HttpCode::BAD_REQUEST;
    }

    bool CheckPath()
    {
        static const std::set<std::string> SUPPORTED_PATHS = {"/index", "/login", "/video", "/picture", "/home"};

        if (path_ == "/") {
            path_ = "/index.html";
            return true;
        }
        if (auto iter = SUPPORTED_PATHS.find(path_); iter == SUPPORTED_PATHS.end()) {
            return false;
        }
        path_ += ".html";
        return true;
    }

    bool IsKeepAlive() const
    {
        if (version_ != "1.1") {
            return false;
        }
        auto iter = header_.find("Connection");
        return iter != header_.end() && iter->second == "keep-alive";
    }

private:
    ParseStatus parseStatus_{ParseStatus::PS_REQUESTLINE};
    
    std::string method_;
    std::string path_;
    std::string version_;
    std::map<std::string, std::string> header_;
    std::map<std::string, std::string> post_;
};

}

#endif