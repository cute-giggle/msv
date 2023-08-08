// Author: cute-giggle@outlook.com

#include "server/server.h"

std::filesystem::path msv::HttpConnection::ResDir;

int main()
{
    mlog::Collection::instance().addStream(std::shared_ptr<mlog::Stream>(new mlog::ConsoleStream));
    msv::HttpConnection::ResDir = "/home/giggle/Workspace/Projects/msv/resources";

    msv::MysqlConfig dbConfig = {
        "localhost",
        3306,
        8,
        "root",
        "123456",
        "user"
    };

    msv::ServerConfig svConfig = {
        2333,
        msv::TriggerMode::TM_ET,
        msv::TriggerMode::TM_ET,
        60000,
        false,
        8,
        dbConfig
    };

    msv::Server server(svConfig);
    server.Start();

    return 0;
}