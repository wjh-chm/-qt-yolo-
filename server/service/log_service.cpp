#include "log_service.h"

#include <mysql/mysql.h>

#include <string>

/**
 * @brief 日志查询服务构造函数
 * @param db 数据库连接对象指针，取自数据库连接池
 */
LogService::LogService(DbConnection* db)
    : m_db(db)
{
}

/**
 * @brief 处理查询日志列表的业务入口
 * @param request 客户端json请求报文
 * 请求报文示例：
 * {
 *   "type":"QUERY_LOG_LIST_REQ",
 *   "data":{
 *       "mode":"exception",   // exception异常日志 / operation操作审计日志
 *       "page":1,
 *       "page_size":8
 *   }
 * }
 * @return json 响应报文，根据mode返回不同类型日志分页数据
 * 错误码约定：
 *      0      : 查询成功
 *      1001   : 请求缺少data业务字段
 *      30xx   : MySQL数据库执行异常
 */
json LogService::handleQueryLogList(const json& request)
{
    json response;
    // 设置消息类型：查询日志列表响应
    response["type"] = "QUERY_LOG_LIST_RESP";
    response["request_id"] = request.value("request_id", 0);
    // 校验请求报文是否包含data业务节点
    if (!request.contains("data")) {
        response["code"] = 1001;
        response["message"] = "missing data";
        return response;
    }

    const json& data = request["data"];

    // mode：exception查询异常日志；operation查询管理员操作审计日志，默认exception
    std::string mode = data.value("mode", "exception");
    int page = data.value("page", 1);                   // 当前页码，默认第1页
    int pageSize = data.value("page_size", 8);          // 每页条数，默认8条

    // 页码容错，不能小于1
    if (page < 1) {
        page = 1;
    }

    // 每页条数安全限制：最小1，最大100，避免一次性读取大量数据
    if (pageSize < 1 || pageSize > 100) {
        pageSize = 8;
    }

    // 根据mode分发到不同查询函数
    if (mode == "operation") {
        // 查询管理员操作审计日志表 operation_log
        return queryOperationLogs(page, pageSize);
    }

    // 默认查询设备异常日志表 exception_log
    return queryExceptionLogs(page, pageSize);
}

/**
 * @brief 查询设备异常日志（采集端设备报错、事件日志）
 * @param page 当前页码
 * @param pageSize 每页条数
 * @return json 异常日志分页响应json
 */
json LogService::queryExceptionLogs(int page, int pageSize)
{
    json response;
    response["type"] = "QUERY_LOG_LIST_RESP";

    // 计算分页偏移量
    int offset = (page - 1) * pageSize;

    // 查询exception_log异常日志表，按事件发生时间倒序，最新日志靠前
    std::string sql =
        "SELECT id, channel_id, event_time, related_videopath "
        "FROM exception_log "
        "ORDER BY event_time DESC "
        "LIMIT " + std::to_string(offset) + "," + std::to_string(pageSize);

    MYSQL* conn = m_db->handle();

    // 执行SQL，非0代表执行出错
    if (mysql_query(conn, sql.c_str()) != 0) {
        response["code"] = 3001;
        response["message"] = mysql_error(conn);
        return response;
    }

    // 将全部查询结果读取到本地内存
    MYSQL_RES* result = mysql_store_result(conn);
    if (result == nullptr) {
        response["code"] = 3002;
        response["message"] = mysql_error(conn);
        return response;
    }

    json list = json::array();

    MYSQL_ROW row;
    // 循环读取每一行异常日志记录
    while ((row = mysql_fetch_row(result)) != nullptr) {
        json item;
        // row判空保护，数据库字段NULL时row[x]为nullptr，防止stoi崩溃
        item["id"] = row[0] ? std::stoi(row[0]) : 0;
        item["channel_id"] = row[1] ? std::stoi(row[1]) : 0;
        item["event_time"] = row[2] ? row[2] : "";
        item["related_videopath"] = row[3] ? row[3] : "";
        list.push_back(item);
    }

    // 释放mysql结果集，防止内存泄漏
    mysql_free_result(result);

    // 组装返回数据，标记mode为exception，告诉前端这是异常日志
    response["code"] = 0;
    response["message"] = "success";
    response["data"]["mode"] = "exception";
    response["data"]["list"] = list;
    response["data"]["page"] = page;
    response["data"]["page_size"] = pageSize;
    response["data"]["total_count"] = queryLogCount("exception_log");

    return response;
}

/**
 * @brief 查询管理员操作审计日志，记录后台管理员所有操作行为
 * @param page 当前页码
 * @param pageSize 每页条数
 * @return json 操作审计日志分页响应json
 */
json LogService::queryOperationLogs(int page, int pageSize)
{
    json response;
    response["type"] = "QUERY_LOG_LIST_RESP";

    // 计算分页偏移量
    int offset = (page - 1) * pageSize;

    // 查询operation_log操作审计日志表，按操作时间倒序
    std::string sql =
        "SELECT id, operation_desc, admin_id, operation_time "
        "FROM operation_log "
        "ORDER BY operation_time DESC "
        "LIMIT " + std::to_string(offset) + "," + std::to_string(pageSize);

    MYSQL* conn = m_db->handle();

    // 执行SQL查询
    if (mysql_query(conn, sql.c_str()) != 0) {
        response["code"] = 3001;
        response["message"] = mysql_error(conn);
        return response;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (result == nullptr) {
        response["code"] = 3002;
        response["message"] = mysql_error(conn);
        return response;
    }

    json list = json::array();

    MYSQL_ROW row;
    // 循环读取每一条管理员操作记录
    while ((row = mysql_fetch_row(result)) != nullptr) {
        json item;
        item["id"] = row[0] ? std::stoi(row[0]) : 0;
        item["operation_desc"] = row[1] ? row[1] : "";
        item["admin_id"] = row[2] ? std::stoi(row[2]) : 0;
        item["operation_time"] = row[3] ? row[3] : "";
        list.push_back(item);
    }

    // 释放mysql结果集内存
    mysql_free_result(result);

    // 组装返回数据，标记mode为operation，告诉前端这是管理员操作日志
    response["code"] = 0;
    response["message"] = "success";
    response["data"]["mode"] = "operation";
    response["data"]["list"] = list;
    response["data"]["page"] = page;
    response["data"]["page_size"] = pageSize;
    response["data"]["total_count"] = queryLogCount("operation_log");

    return response;
}
int LogService::queryLogCount(const std::string &tableName)
{
    std::string sql = "SELECT COUNT(*) FROM " + tableName;

    MYSQL* conn = m_db->handle();

    if (mysql_query(conn, sql.c_str()) != 0) {
        return 0;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (result == nullptr) {
        return 0;
    }

    MYSQL_ROW row = mysql_fetch_row(result);

    int count = 0;
    if (row && row[0]) {
        count = std::stoi(row[0]);
    }

    mysql_free_result(result);
    return count;
}