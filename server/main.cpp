#include "db/db_connection.h"
#include "net/tcp_server.h"
#include "util/config.h"

#include <iostream>

int main()
{
    Config config;

    if (!config.load("../config/db.ini")) {
        std::cout << "load db config failed" << std::endl;
        return 1;
    }

    DbConnection db;

    if (!db.connect(config)) {
        return 1;
    }

    std::cout << "mysql connect success" << std::endl;

    TcpServer server(&db);

    if (!server.start(9000)) {
        std::cout << "server start failed" << std::endl;
        return 1;
    }

    return 0;
}