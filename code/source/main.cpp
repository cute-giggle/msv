// Author: cute-giggle@outlook.com

#include "server/server.h"

int main()
{
    mlog::Collection::instance().addStream(std::shared_ptr<mlog::Stream>(new mlog::ConsoleStream));
    mlog::Collection::instance().setLevel(mlog::Level::L_DEBUG);

    msv::HttpConnection::ResDir = "/home/giggle/Workspace/Projects/msv/resources";

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
        msv::TriggerMode::TM_ET,
        10000,
        false,
        8,
        dbConfig
    };

    msv::Server server(svConfig);
    server.Start();

    return 0;
}