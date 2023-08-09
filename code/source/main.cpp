// Author: cute-giggle@outlook.com

#include "server/server.h"

std::filesystem::path msv::HttpConnection::ResDir = "/home/giggle/Workspace/Projects/msv/resources";
std::atomic<uint32_t> msv::HttpConnection::NumOnline = 0;

int main()
{
    mlog::Collection::instance().addStream(std::shared_ptr<mlog::Stream>(new mlog::ConsoleStream));
    mlog::Collection::instance().setLevel(mlog::Level::L_DEBUG);

    msv::MysqlConfig dbConfig = {
        "localhost",
        3306,
        8,
        "giggle",
        "123456",
        "user"
    };

    msv::ServerConfig svConfig = {
        2333,
        msv::TriggerMode::TM_ET,
        msv::TriggerMode::TM_LT,
        10000,
        false,
        8,
        dbConfig
    };

    msv::Server server(svConfig);
    server.Start();

    return 0;
}