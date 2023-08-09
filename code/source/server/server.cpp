// Author: cute-giggle@outlook.com

#include "server/server.h"

namespace msv {

Server::Server(ServerConfig config)
{
    port_ = config.serverPort;
    epTrigMode_ = config.epTrigMode;
    cnTrigMode_ = config.cnTrigNode;
    cnTimeout_ = config.cnTimeout;
    optLinger_ = config.optLinger;
    threadPool_.Initialize(config.numThread);
    MysqlPool::InitInstance(config.mysqlConfig);
    InitializeEvents();
    if (!InitializeSocket())
    {
        shutdown_ = true;
    }
}

void Server::Start()
{
    MLOG_INFOR("Server start!");
    while (!shutdown_) {
        auto epTimeout = timeNodeHeap_.GetMinTimeout();

        MLOG_DEBUG("Min epoll timeout: ", epTimeout);

        auto numEvents = epoller_.Wait(static_cast<int>(epTimeout));
        for (auto i = 0; i < numEvents; ++i) {
            auto events = epoller_[i].events;
            auto fd = epoller_[i].data.fd;
            if (fd == sfd_) {
                Listen();
            }
            else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                CloseConnection(&userMapping_[fd]);
            }
            else if (events & (EPOLLIN)) {
                timeNodeHeap_.Modify(fd, cnTimeout_);
                threadPool_.AddTask(std::bind(&Server::ReadEntry, this, &userMapping_[fd]));
            }
            else if (events & (EPOLLOUT)) {
                timeNodeHeap_.Modify(fd, cnTimeout_);
                threadPool_.AddTask(std::bind(&Server::WriteEntry, this, &userMapping_[fd]));
            }
            else {
                MLOG_ERROR("Unexpected epoll events: ", events);
            }
        }
    }
}

void Server::ReadEntry(HttpConnection *conn)
{
    if (!conn->Read()) {
        MLOG_ERROR("Read error");
        CloseConnection(conn);
        return;
    }
    ProcessEntry(conn);
}

void Server::ProcessEntry(HttpConnection *conn)
{
    if (conn->Process()) {
        epoller_.ModFd(conn->GetFd(), connectionEvents_ | EPOLLOUT);
    }
    else {
        epoller_.ModFd(conn->GetFd(), connectionEvents_ | EPOLLIN);
    }
}

void Server::WriteEntry(HttpConnection* conn)
{
    if (!conn->Write()) {
        MLOG_ERROR("Write error");
        CloseConnection(conn);
        return;
    }
    if (!conn->WriteComplete()) {
        epoller_.ModFd(conn->GetFd(), connectionEvents_ | EPOLLOUT);
        return;
    }
    if (conn->IsKeepAlive()) {
        ProcessEntry(conn);
        return;
    }
    MLOG_DEBUG("Write complete and no keep-alive");
    CloseConnection(conn);
}

void Server::CloseConnection(HttpConnection *conn)
{
    if (conn->IsClosed()) {
        return;
    }
    MLOG_DEBUG("Connection closed! fd: ", conn->GetFd());
    epoller_.DelFd(conn->GetFd());
    conn->Close();
}

void Server::SendError(int cfd, const char *info) const
{
    auto len = send(cfd, info, strlen(info), 0);
    if (len < 0) {
        MLOG_ERROR("Send error info failed! fd: ", cfd);
    }
    close(cfd);
}

void Server::Listen()
{
    struct sockaddr_in addr {0};
    socklen_t len = sizeof(addr);
    do {
        auto cfd = accept(sfd_, reinterpret_cast<sockaddr *>(&addr), &len);
        if (cfd < 0) {
            return;
        }
        if (HttpConnection::NumOnline >= MAX_CONNECTION_NUM) {
            MLOG_DEBUG("Server busy! online number: ", HttpConnection::NumOnline);
            SendError(cfd, "Server busy!");
            return;
        }
        HttpConnection *conn = &(userMapping_[cfd]);
        conn->Initialize(cnTrigMode_, cfd, addr);
        timeNodeHeap_.Insert(cfd, cnTimeout_, std::bind(&Server::CloseConnection, this, conn));
        epoller_.AddFd(cfd, connectionEvents_ | EPOLLIN);
        fcntl(cfd, F_SETFL, fcntl(cfd, F_GETFD, 0) | O_NONBLOCK);
    } while (listenEvents_ & EPOLLET);
}

bool Server::InitializeSocket()
{
    if (port_ < 1024) {
        MLOG_ERROR("Error server port: ", port_);
        return false;
    }

    struct sockaddr_in addr = {0};
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_family = AF_INET;
    sfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd_ < 0) {
        MLOG_ERROR("Create server socket failed!");
        return false;
    }

    struct linger lg = {0};
    if (optLinger_) {
        lg.l_onoff = 1;
        lg.l_linger = 1;
    }
    if (auto ret = setsockopt(sfd_, SOL_SOCKET, SO_LINGER, reinterpret_cast<const void *>(&lg), sizeof(lg)); ret < 0) {
        MLOG_ERROR("Set linger option failed!");
        close(sfd_);
        return false;
    }

    int reuse = 1;
    if (auto ret = setsockopt(sfd_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const void *>(&reuse), sizeof(reuse)); ret < 0) {
        MLOG_ERROR("Set addr reuse failed!");
        close(sfd_);
        return false;
    }

    if (auto ret = bind(sfd_, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)); ret < 0) {
        MLOG_ERROR("Bind server socket failed!");
        close(sfd_);
        return false;
    }

    if (auto ret = listen(sfd_, 6); ret < 0) {
        MLOG_ERROR("Listen socket failed!");
        close(sfd_);
        return false;
    }

    if (!epoller_.AddFd(sfd_, listenEvents_ | EPOLLIN)) {
        MLOG_ERROR("Epoll add listen events failed!");
        close(sfd_);
        return false;
    }

    fcntl(sfd_, F_SETFL, fcntl(sfd_, F_GETFD, 0) | O_NONBLOCK);
    return true;
}

void Server::InitializeEvents()
{
    listenEvents_ = EPOLLRDHUP;
    connectionEvents_ = EPOLLONESHOT | EPOLLRDHUP;

    if (epTrigMode_ == TriggerMode::TM_ET) {
        listenEvents_ |= EPOLLET;
    }
    if (cnTrigMode_ == TriggerMode::TM_ET) {
        listenEvents_ |= EPOLLET;
    }
}

}