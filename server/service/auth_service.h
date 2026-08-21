#pragma once

#include "db/db_connection.h"
#include "protocol/protocol.h"

class AuthService {
public:
    explicit AuthService(DbConnection* db);

    json handleLogin(const json& request);

private:
    DbConnection* m_db = nullptr;
};