// Author: cute-giggle@outlook.com

#ifndef MLOG_H
#define MLOG_H

#include <mutex>
#include <atomic>
#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <memory>
#include <iostream>

namespace mlog
{

class Stream
{
public:
    virtual void write(const std::string& msg) noexcept = 0;

    virtual ~Stream() = default;
};

class ConsoleStream : public Stream
{
public:
    void write(const std::string& msg) noexcept override
    {
        std::cout << msg << std::endl;
    }

    virtual ~ConsoleStream() = default;
};

class FileStream : public Stream
{
public:
    static constexpr uint32_t MAX_CAPACITY = 1U << 24;

    FileStream(const std::filesystem::path& path, uint32_t capacity = (1U << 20)) noexcept
        : path_(path), capacity_(std::min(capacity, MAX_CAPACITY)), output_(path, std::ios_base::app) {}
    
    virtual ~FileStream() = default;

    void write(const std::string& msg) noexcept override
    {
        if (output_.fail())
        {
            return;
        }
        if (output_.tellp() > capacity_)
        {
            output_.close();
            output_.open(path_, std::ios::out | std::ios::trunc);
        }
        output_ << msg << std::endl;
    }

private:
    uint32_t capacity_{};
    std::filesystem::path path_;
    std::ofstream output_;
};

enum class Level
{
    L_DEBUG,
    L_INFOR,
    L_WARNN,
    L_ERROR,
    L_FATAL,
    L_CLOSE,
};

class Collection
{
public:
    static Collection& instance() noexcept
    {
        static Collection instance_;
        return instance_;
    }

    void setLevel(Level newLevel) noexcept
    {
        level_ = newLevel;
    }

    Level getLevel() const noexcept
    {
        return level_;
    }

    void addStream(std::shared_ptr<Stream>&& stream) noexcept
    {
        std::lock_guard<std::mutex> lock(mtx_);
        list_.emplace_back(std::move(stream));
    }

    template<Level L, typename... Args>
    void write(Args&&... args) noexcept
    {
        if (L >= level_)
        {
            std::stringstream buffer;
            (buffer << ... << std::forward<Args>(args));
            auto operation = [&buffer](std::shared_ptr<Stream>& stream) { stream->write(buffer.str()); };
            std::lock_guard<std::mutex> lock(mtx_);
            std::for_each(list_.begin(), list_.end(), operation);
        }
    }

private:
    explicit Collection(Level level = Level::L_INFOR) noexcept : mtx_(), level_(level), list_() {}

    Collection(const Collection&) = delete;
    Collection& operator= (const Collection&) = delete;

    std::mutex mtx_;
    std::atomic<Level> level_;
    std::vector<std::shared_ptr<Stream>> list_;
};

template<typename... Args>
inline std::string format(const char* fmt, Args&&... args) noexcept
{
    static constexpr uint32_t MAX_BUFFER_LENGTH = 128U;
    static char buffer[MAX_BUFFER_LENGTH]{};
    snprintf(buffer, MAX_BUFFER_LENGTH, fmt, std::forward<Args>(args)...);
    return buffer;
}

}

#define MLOG_EXT_INFO __DATE__, ": ", __TIME__, ": ", __FILE__, ":", __LINE__, ": "

#define MLOG_DEBUG(msg...) mlog::Collection::instance().write<mlog::Level::L_DEBUG>("DEBUG: ", MLOG_EXT_INFO, msg)

#define MLOG_INFOR(msg...) mlog::Collection::instance().write<mlog::Level::L_INFOR>("INFOR: ", MLOG_EXT_INFO, msg)

#define MLOG_WARNN(msg...) mlog::Collection::instance().write<mlog::Level::L_WARNN>("WARNN: ", MLOG_EXT_INFO, msg)

#define MLOG_ERROR(msg...) mlog::Collection::instance().write<mlog::Level::L_ERROR>("ERROR: ", MLOG_EXT_INFO, msg)

#define MLOG_FATAL(msg...) mlog::Collection::instance().write<mlog::Level::L_FATAL>("FATAL: ", MLOG_EXT_INFO, msg)

#endif