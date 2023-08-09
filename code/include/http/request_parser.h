// Author: cute-giggle@outlook.com

#ifndef REQUEST_PARSER_H
#define REQUEST_PARSER_H

#include <map>
#include <set>
#include <regex>

#include "read_buffer.h"
#include "db/mysqlpool.h"

namespace msv {

namespace http {

class RequestParser {
public:
    enum class ParseStatus: uint8_t {
        REQUESTLINE = 0U,
        HEADER,
        BODY,
        FINISH,
    };

    enum class RetStatus : uint8_t {
        NO_REQUEST = 0U,
        GET_REQUEST,
        BAD_REQUEST,
    };

public:
    RequestParser() = default;

    ~RequestParser()
    {
        Reset();
    }

    void Reset();

    bool ParseRequestLine(const std::string& line);

    void FormatPath();

    bool ParseHeader(const std::string& line);

    bool ParseBody(const std::string& line);

    void UserVerify();

    RetStatus Parse(ReadBuffer& rdbuf);

    bool IsKeepAlive() const
    {
        if (version_ != "1.1") {
            return false;
        }
        auto iter = header_.find("Connection");
        return iter != header_.end() && iter->second == "keep-alive";
    }

    std::string GetPath() const
    {
        return path_;
    }

private:
    ParseStatus parseStatus_{ParseStatus::REQUESTLINE};
    
    std::string method_;
    std::string path_;
    std::string version_;
    std::map<std::string, std::string> header_;
    std::map<std::string, std::string> body_;
};

}

}

#endif