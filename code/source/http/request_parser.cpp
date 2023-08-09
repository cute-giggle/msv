// Author: cute-giggle@outlook.com

#include "http/request_parser.h"

namespace msv::http {

bool RequestParser::ParseRequestLine(const std::string &line)
{
    std::regex expr("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch result;
    if (std::regex_match(line, result, expr)) {
        method_ = std::move(result[1]);
        path_ = std::move(result[2]);
        version_ = std::move(result[3]);
        return method_ == "GET" || method_ == "POST";
    }
    return false;
}

void RequestParser::Reset()
{
    parseStatus_ = ParseStatus::REQUESTLINE;
    method_.clear();
    path_.clear();
    version_.clear();
    header_.clear();
    body_.clear();
}

void RequestParser::FormatPath()
{
    static const std::set<std::string> urls = {"/index", "/register", "/login", "/home", "/image", "/video"};

    if (*(path_.rbegin()) == '/') {
        path_.pop_back();
    }

    if (path_.empty()) {
        path_ = "/index.html";
    } else if (urls.count(path_)) {
        path_ += ".html";
    }
}

bool RequestParser::ParseHeader(const std::string &line)
{
    std::regex expr("^([^:]*): ?(.*)$");
    std::smatch result;
    if (std::regex_match(line, result, expr)) {
        header_[result[1]] = result[2];
        return true;
    }
    return false;
}

bool RequestParser::ParseBody(const std::string &line)
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
    while (!ss.eof()) {
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

void RequestParser::UserVerify()
{
    bool tryLogin = (path_ == "/login.html");
    auto mysqlConn = GetMysqlConnection();

    const auto &username = body_["username"];
    const auto &password = body_["password"];

    char order[256] = {0};
    snprintf(order, sizeof(order), "SELECT username, password FROM tb_auth WHERE username='%s' LIMIT 1", username.c_str());

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
            path_ = "/home.html";
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
    snprintf(order, sizeof(order), "INSERT INTO tb_auth(username, password) VALUES('%s','%s')", username.c_str(), password.c_str());
    if (mysql_query(mysqlConn.get(), order)) {
        path_ = "/error.html";
        mysql_free_result(result);
        return;
    }
    path_ = "/home.html";
    mysql_free_result(result);
}

RequestParser::RetStatus RequestParser::Parse(ReadBuffer &rdbuf)
{
    while (parseStatus_ != ParseStatus::FINISH) {
        std::optional<std::string> ret = std::nullopt;

        if (parseStatus_ == ParseStatus::BODY) {
            std::size_t length = std::stoi(header_["Content-Length"]);
            if (length <= rdbuf.Size()) {
                ret = rdbuf.GetBytes(length);
            }
        }
        else {
            ret = rdbuf.GetLine();
        }

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
            continue;
        }

        switch (parseStatus_) {
        case ParseStatus::REQUESTLINE:
            if (!ParseRequestLine(line)) {
                MLOG_DEBUG("Parse request line failed!");
                return RetStatus::BAD_REQUEST;
            }
            FormatPath();
            parseStatus_ = ParseStatus::HEADER;
            break;
        case ParseStatus::HEADER:
            if (!ParseHeader(line)) {
                MLOG_DEBUG("Parse header failed!");
                return RetStatus::BAD_REQUEST;
            }
            break;
        case ParseStatus::BODY:
            if (!ParseBody(line)) {
                MLOG_DEBUG("Parse body failed!");
                return RetStatus::BAD_REQUEST;
            }
            UserVerify();
            parseStatus_ = ParseStatus::FINISH;
            break;
        default:
            MLOG_DEBUG("Parse unknown error!");
            return RetStatus::BAD_REQUEST;
            break;
        }
    }
    return RetStatus::GET_REQUEST;
}
}