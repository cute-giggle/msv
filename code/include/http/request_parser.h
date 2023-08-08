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

    void Reset()
    {
        parseStatus_ = ParseStatus::REQUESTLINE;

        method_.clear();
        path_.clear();
        version_.clear();
        header_.clear();
        body_.clear();
    }

    bool ParseRequestLine(const std::string& line)
    {
        std::regex expr("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
        std::smatch result;
        if (std::regex_match(line, result, expr)) {
            method_ = std::move(result[0]);
            path_ = std::move(result[1]);
            version_ = std::move(result[2]);
            return method_ == "GET" || method_ == "POST";
        }
        return false;
    }

    void FormatPath()
    {
        static const std::set<std::string> urls = {"/index", "/register", "/login", "/welcome", "/video", "/picture"};

        if (*(path_.rbegin()) == '/') {
            path_.pop_back();
        }

        if (path_.empty()) {
            path_ = "/index.html";
        } else if (urls.count(path_)) {
            path_ += ".html";
        }
    }

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

    bool ParseBody(const std::string& line)
    {
        if (path_ != "/login.html" && path_ != "/register.html") {
            return false;
        }
        if (method_ != "POST" || header_["Content-Type"] != "application/x-www-form-urlencoded") {
            return false;
        }

        auto str = line;
        std::replace(str.begin(), str.end(), '&', ' ');
        std::replace(str.begin(), str.end(), '=', ' ');
        std::stringstream ss(str);
        while (!ss.eof())
        {
            std::string key, value;
            ss >> key >> value;
            std::replace(key.begin(), key.end(), '+', ' ');
            std::replace(value.begin(), value.end(), '+', ' ');
            body_[key] = value;
        }

        if (!body_.count("username") || !body_.count("password")) {
            return false;
        }

        return true;
    }

    void UserVerify()
    {
        bool tryLogin = (path_ == "/login.html");
        auto mysqlConn = GetMysqlConnection();

        const auto& username = body_["username"];
        const auto& password = body_["password"];

        char order[256] = {0};
        snprintf(order, sizeof(order), "SELECT username, password FROM user WHERE username='%s' LIMIT 1", username.c_str());

        if (mysql_query(mysqlConn.get(), order)) {
            path_ = "/error.html";
            return;
        }

        auto result = mysql_store_result(mysqlConn.get());
        auto numRows = mysql_num_rows(result);
        
        // login but user not exist
        if (tryLogin && numRows == 0) {
            path_ = "/error.html";
            mysql_free_result(result);
            return;
        }

        if (tryLogin) {
            auto row = mysql_fetch_row(result);
            std::string realPassword = row[1];
            if (password == realPassword) {
                path_ = "/welcome.html";
                mysql_free_result(result);
                return;
            }
            path_ = "/error.html";
            mysql_free_result(result);
            return;
        }

        // register but user already exist
        if (numRows) {
            path_ = "/error.html";
            mysql_free_result(result);
            return;
        }

        bzero(order, sizeof(order));
        snprintf(order, sizeof(order),"INSERT INTO user(username, password) VALUES('%s','%s')", username.c_str(), password.c_str());
        if (mysql_query(mysqlConn.get(), order)) {
            path_ = "/error.html";
            mysql_free_result(result);
            return;
        }
        path_ = "/welcome.html";
        mysql_free_result(result);
    }

    RetStatus Parse(ReadBuffer& rdbuf)
    {
        while (parseStatus_ != ParseStatus::FINISH) {
            auto ret = rdbuf.GetLine();
            if (ret == std::nullopt) {
                return RetStatus::NO_REQUEST;
            }
            auto line = std::move(ret.value());

            if (parseStatus_ == ParseStatus::HEADER && line.empty()) {
                if (method_ == "POST") {
                    parseStatus_ = ParseStatus::BODY;
                    continue;
                }
                parseStatus_ = ParseStatus::FINISH;
                continue;;
            }

            switch (parseStatus_) {
            case ParseStatus::REQUESTLINE:
                if (!ParseRequestLine(line)) {
                    return RetStatus::BAD_REQUEST;
                }
                FormatPath();
                parseStatus_ = ParseStatus::HEADER;
                break;
            case ParseStatus::HEADER:
                if (!ParseHeader(line)) {
                    return RetStatus::BAD_REQUEST;
                }
                break;
            case ParseStatus::BODY:
                if (!ParseBody(line)) {
                    return RetStatus::BAD_REQUEST;
                }
                UserVerify();
                parseStatus_ = ParseStatus::FINISH;
                break;
            default:
                return RetStatus::BAD_REQUEST;
                break;
            }
        }
        return RetStatus::GET_REQUEST;
    }

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