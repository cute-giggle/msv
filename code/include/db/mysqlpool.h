// Author: cute-giggle@outlook.com

#ifndef MYSQLPOOL_H
#define MYSQLPOOL_H

#include <string>
#include <queue>
#include <mutex>
#include <memory>
#include <condition_variable>
#include <mysql/mysql.h>

#include "utils/mlog.h"

namespace msv {

struct MysqlConfig {
    std::string host;
    uint16_t port;
    uint16_t numConnect;
    std::string user;
    std::string passwd;
    std::string dbname;
};

class MysqlPool {
public:
    static constexpr uint16_t MAX_NUM_CONNECT = 32U;

    static MysqlPool* Instance()
    {
        static MysqlPool pool;
        return &pool;
    }

    static bool InitInstance(const MysqlConfig& config)
    {
        return Instance()->Initialize(config);
    }

    ~MysqlPool()
    {
        Shutdown();
    }

    MYSQL* GetConnection()
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

    void RetConnection(MYSQL* ptr)
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

    void Shutdown()
    {
        std::lock_guard locker(mutex_);
        while(!queue_.empty()) {
            auto ptr = queue_.front();
            queue_.pop();
            mysql_close(ptr);
        }
    }

private:
    MysqlPool() = default;

    MysqlPool(const MysqlPool& rhs) = delete;
    MysqlPool& operator=(const MysqlPool& rhs) = delete;

    [[nodiscard]] bool Initialize(const MysqlConfig& config)
    {
        config_ = config;
        config_.numConnect = std::min(config_.numConnect, MAX_NUM_CONNECT);

        for (auto i = 0U; i < numConnect_; ++i) {
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
        return true;
    }

private:
    MysqlConfig config_;
    std::mutex mutex_;
    uint32_t numConnect_;
    std::queue<MYSQL*> queue_;
    std::condition_variable cond_;
};

inline auto GetMysqlConnection()
{
    using DelFuncType = std::function<void(MYSQL*)>;
    return std::unique_ptr<MYSQL, DelFuncType>(MysqlPool::Instance()->GetConnection(), [](MYSQL* ptr) { MysqlPool::Instance()->RetConnection(ptr); });
}

}

#endif