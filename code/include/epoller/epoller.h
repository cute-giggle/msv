// Author: cute-giggle@outlook.com

#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h>
#include <unistd.h>
#include <vector>

namespace msv {

class Epoller {
public:
    static constexpr uint32_t MAX_EVENT_BUFFER_SIZE = 4096U;

public:
    explicit Epoller(uint32_t eventBufferSize = 4096U) : efd_(epoll_create(2333)), buffer(std::min(eventBufferSize, MAX_EVENT_BUFFER_SIZE))
    {

    }

    ~Epoller()
    {
        [[maybe_unused]] auto ret = close(efd_);
    }

    bool AddFd(int fd, uint32_t events)
    {
        union epoll_data ed{0};
        ed.fd = fd;
        struct epoll_event ee{events, ed};
        return 0 == epoll_ctl(efd_, EPOLL_CTL_ADD, fd, &ee);
    }

    bool ModFd(int fd, uint32_t events)
    {
        union epoll_data ed{0};
        ed.fd = fd;
        struct epoll_event ee{events, ed};
        return 0 == epoll_ctl(efd_, EPOLL_CTL_MOD, fd, &ee);
    }

    bool DelFd(int fd)
    {
        struct epoll_event ee{0};
        return 0 == epoll_ctl(efd_, EPOLL_CTL_DEL, fd, &ee);
    }

    int Wait(int timeout)
    {
        return epoll_wait(efd_, buffer.data(), buffer.size(), timeout);
    }

    const struct epoll_event& operator[](std::size_t index) const
    {
        return buffer[index];
    }

private:
    int efd_;
    std::vector<struct epoll_event> buffer;
};

}

#endif