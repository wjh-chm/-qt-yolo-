#pragma once

#include "util/config.h"

#include <mysql/mysql.h>

class DbConnection {
public:
    DbConnection();
    ~DbConnection();

    bool connect(const Config& config);
    bool isOpen() const;
    bool testSelectOne();
    MYSQL* handle();

private:
    MYSQL* m_conn = nullptr;
};