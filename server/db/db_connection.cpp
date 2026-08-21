#include "db_connection.h"

#include <iostream>

DbConnection::DbConnection()
{
    m_conn = mysql_init(nullptr);
}

DbConnection::~DbConnection()
{
    if (m_conn != nullptr) {
        mysql_close(m_conn);
        m_conn = nullptr;
    }
}
MYSQL* DbConnection::handle()
{
    return m_conn;
}

bool DbConnection::connect(const Config& config)
{
    if (m_conn == nullptr) {
        std::cout << "mysql_init failed" << std::endl;
        return false;
    }

    MYSQL* ret = mysql_real_connect(
        m_conn,
        config.dbHost().c_str(),
        config.dbUser().c_str(),
        config.dbPassword().c_str(),
        config.dbName().c_str(),
        config.dbPort(),
        nullptr,
        0
    );

    if (ret == nullptr) {
        std::cout << "mysql connect failed: " << mysql_error(m_conn) << std::endl;
        return false;
    }

    mysql_set_character_set(m_conn, "utf8mb4");
    return true;
}

bool DbConnection::isOpen() const
{
    return m_conn != nullptr;
}

bool DbConnection::testSelectOne()
{
    const char* sql = "SELECT 1";

    if (mysql_query(m_conn, sql) != 0) {
        std::cout << "query failed: " << mysql_error(m_conn) << std::endl;
        return false;
    }

    MYSQL_RES* result = mysql_store_result(m_conn);
    if (result == nullptr) {
        std::cout << "store result failed: " << mysql_error(m_conn) << std::endl;
        return false;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    if (row != nullptr) {
        std::cout << "select result: " << row[0] << std::endl;
    }

    mysql_free_result(result);
    return true;
}