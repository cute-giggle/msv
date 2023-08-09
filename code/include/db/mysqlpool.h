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
    std::string host{"localhost"};
    uint16_t port{3306U};
    uint16_t numConnect{8U};
    std::string user{};
    std::string passwd{};
    std::string dbname{};
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

    MYSQL* GetConnection();
    void RetConnection(MYSQL* ptr);

    void Shutdown();

private:
    MysqlPool() = default;

    MysqlPool(const MysqlPool& rhs) = delete;
    MysqlPool& operator=(const MysqlPool& rhs) = delete;

    [[nodiscard]] bool Initialize(const MysqlConfig& config);

private:
    MysqlConfig config_{};
    std::mutex mutex_{};
    std::queue<MYSQL*> queue_{};
    std::condition_variable cond_{};
};

inline auto GetMysqlConnection()
{
    using DelFuncType = std::function<void(MYSQL*)>;
    return std::unique_ptr<MYSQL, DelFuncType>(MysqlPool::Instance()->GetConnection(), [](MYSQL* ptr) { MysqlPool::Instance()->RetConnection(ptr); });
}

}

#endif