// Author: cute-giggle@outlook.com

#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <fcntl.h>

#include "db/mysqlpool.h"
#include "threadpool/threadpool.h"
#include "epoller/epoller.h"
#include "http/http_connection.h"
#include "timeout.h"

namespace msv {

using namespace http;
using UserMapping = std::unordered_map<int, HttpConnection>;

struct ServerConfig {
    uint16_t serverPort{2333U};
    TriggerMode epTrigMode{TriggerMode::TM_ET};
    TriggerMode cnTrigNode{TriggerMode::TM_ET};
    uint32_t cnTimeout{60000U};
    bool optLinger{false};
    uint32_t numThread{8U};
    MysqlConfig mysqlConfig{};
};

class Server {
public:
    static constexpr std::size_t MAX_CONNECTION_NUM = 65535U;

public:
    Server(ServerConfig config);
    
    ~Server()
    {
        Shutdown();
    }

    void Start();

    void Shutdown()
    {
        close(sfd_);
        shutdown_ = true;
        threadPool_.Shutdown();
    }

    void ReadEntry(HttpConnection* conn);
    void ProcessEntry(HttpConnection* conn);
    void WriteEntry(HttpConnection* conn);

    void CloseConnection(HttpConnection* conn);

    void SendError(int cfd, const char* info) const;

    void Listen();

    bool InitializeSocket();
    void InitializeEvents();

private:
    uint16_t port_;
    TriggerMode epTrigMode_;
    TriggerMode cnTrigMode_;
    TimeStamp cnTimeout_;
    bool optLinger_;
    bool shutdown_;

    int sfd_;
    uint32_t listenEvents_;
    uint32_t connectionEvents_;

    TimeNodeHeap timeNodeHeap_;
    ThreadPool threadPool_;
    Epoller epoller_;

    UserMapping userMapping_;
};

}

#endif