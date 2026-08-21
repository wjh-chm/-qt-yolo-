#pragma once

#include "db/db_connection.h"
#include "protocol/protocol.h"

class LogService {
public:
    explicit LogService(DbConnection* db);

    json handleQueryLogList(const json& request);

private:
    json queryOperationLogs(int page, int pageSize);
    json queryExceptionLogs(int page, int pageSize);
    int queryLogCount(const std::string &tableName);

private:
    DbConnection* m_db = nullptr;
};