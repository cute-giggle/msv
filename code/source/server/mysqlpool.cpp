// Author: cute-giggle@outlook.com

#include "db/mysqlpool.h"

namespace msv {

MYSQL* MysqlPool::GetConnection()
{
    std::unique_lock locker(mutex_);
    if (queue_.empty()) {
        cond_.wait(locker);
    }
    if (!queue_.empty()) {
        auto ret = queue_.front();
        queue_.pop();
        return ret;
    }
    return nullptr;
}

void MysqlPool::RetConnection(MYSQL* ptr)
{
    if (ptr == nullptr) {
        return;
    }

    {
        std::unique_lock locker(mutex_);
        queue_.push(ptr);
    }
    cond_.notify_one();
}

void MysqlPool::Shutdown()
{
    {
        std::unique_lock locker(mutex_);
        while (!queue_.empty()) {
            auto ptr = queue_.front();
            queue_.pop();
            mysql_close(ptr);
        }
    }
    cond_.notify_all();
}

bool MysqlPool::Initialize(const MysqlConfig& config)
{
    config_ = config;
    config_.numConnect = std::min(config_.numConnect, MAX_NUM_CONNECT);

    for (auto i = 0U; i < config_.numConnect; ++i) {
        auto ptr = mysql_init(nullptr);
        if (ptr == nullptr) {
            MLOG_ERROR("Mysql initialize failed!");
            return false;
        }
        ptr = mysql_real_connect(ptr, config_.host.c_str(), config_.user.c_str(), config_.passwd.c_str(), config_.dbname.c_str(), config_.port, nullptr, 0);
        if (ptr == nullptr) {
            MLOG_ERROR("Mysql connected failed!");
            return false;
        }
        queue_.push(ptr);
    }
    MLOG_INFOR("Mysqlpool initialize success!");
    return true;
}

}