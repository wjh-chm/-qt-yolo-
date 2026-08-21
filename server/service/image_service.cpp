#include "image_service.h"

#include <mysql/mysql.h>

#include <string>

/**
 * @brief 图像记录查询服务构造函数
 * @param db 数据库连接对象指针，取自数据库连接池
 */
ImageService::ImageService(DbConnection* db)
    : m_db(db)
{
}

int ImageService::queryImageCount(const std::string& startTime,
                                  const std::string& endTime,
                                  int channelId,
                                  const std::string& scope)
{
    std::string sql =
        "SELECT COUNT(*) "
        "FROM feature_image "
        "WHERE capture_time >= '" + startTime + "' "
        "AND capture_time <= '" + endTime + "' ";

    if (channelId > 0) {
        sql += "AND channel_id = " + std::to_string(channelId) + " ";
    }

    if (scope == "normal") {
        sql += "AND exception_id IS NULL ";
    } else if (scope == "anomaly") {
        sql += "AND exception_id IS NOT NULL ";
    }

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
    if (row != nullptr && row[0] != nullptr) {
        count = std::stoi(row[0]);
    }

    mysql_free_result(result);
    return count;
}

/**
 * @brief 处理查询特征图片列表业务接口（舌苔采集图片记录）
 * @param request 客户端json请求报文
 * 请求报文示例：
 * {
 *   "type":"QUERY_IMAGE_LIST_REQ",
 *   "data":{
 *       "scope":"normal",    // normal正常图片 / anomaly异常图片
 *       "date":"2026‑08‑17", // 查询日期 yyyy‑MM‑dd
 *       "channel_id":0,      // 设备通道id，0代表不过滤通道
 *       "page":1,            // 页码
 *       "page_size":9        // 每页返回条数
 *   }
 * }
 * @return json 响应报文，返回分页图片记录数组
 * 错误码约定：
 *      0      : 查询成功
 *      1001   : 请求缺少data业务字段
 *      1002   : date日期参数为空
 *      30xx   : MySQL数据库执行异常
 */
json ImageService::handleQueryImageList(const json& request)
{
    json response;
    // 设置消息类型：查询图片列表响应
    response["type"] = "QUERY_IMAGE_LIST_RESP";
    response["request_id"] = request.value("request_id", 0);
    // 校验请求报文是否存在data业务数据节点
    if (!request.contains("data")) {
        response["code"] = 1001;
        response["message"] = "missing data";
        return response;
    }

    const json& data = request["data"];

    // 提取请求参数，value第二个参数为取不到参数时的默认值
    std::string scope = data.value("scope", "normal");   // normal正常 / anomaly异常
    std::string date = data.value("date", "");           // 查询日期
    int channelId = data.value("channel_id", 0);        // 采集设备通道id
    int page = data.value("page", 1);                   // 当前页码，默认第1页
    int pageSize = data.value("page_size", 9);           // 每页条数，默认9条

    // 日期为必传参数，为空直接返回错误
    if (date.empty()) {
        response["code"] = 1002;
        response["message"] = "date empty";
        return response;
    }

    // 页码容错：页码不能小于1，小于1强制置为第1页
    if (page < 1) {
        page = 1;
    }

    // 每页条数安全限制：最小1条，最大100条，防止一次性拉取巨量数据
    if (pageSize < 1 || pageSize > 100) {
        pageSize = 9;
    }

    // 计算分页偏移量：offset = (页码‑1) * 每页数量
    int offset = (page - 1) * pageSize;

    // 拼接时间范围，查询当天 00:00:00 ~ 23:59:59 的图片记录
    std::string startTime = date + " 00:00:00";
    std::string endTime = date + " 23:59:59";

    // SQL基础语句，查询feature_image舌苔特征图片表的指定字段
    std::string sql =
        "SELECT id, channel_id, image_name, image_path, capture_time, create_time, exception_id "
        "FROM feature_image "
        "WHERE capture_time >= '" + startTime + "' "
        "AND capture_time <= '" + endTime + "' ";

    // 如果channelId大于0，追加条件，只查询指定采集通道的图片
    if (channelId > 0) {
        sql += "AND channel_id = " + std::to_string(channelId) + " ";
    }

    // scope筛选逻辑
    // normal：正常图片，exception_id为NULL；anomaly：异常图片，exception_id不为NULL
    if (scope == "normal") {
        sql += "AND exception_id IS NULL ";
    } else if (scope == "anomaly") {
        sql += "AND exception_id IS NOT NULL ";
    }

    // 按图片采集时间倒序，最新采集排在最前面；拼接LIMIT实现分页
    sql +=
        "ORDER BY capture_time DESC "
        "LIMIT " + std::to_string(offset) + "," + std::to_string(pageSize);

    // 获取数据库原生操作句柄
    MYSQL* conn = m_db->handle();

    // 执行SQL查询，返回非0代表SQL执行出错
    if (mysql_query(conn, sql.c_str()) != 0) {
        response["code"] = 3001;
        response["message"] = mysql_error(conn);
        return response;
    }

    // 将查询结果全部读取到程序内存
    MYSQL_RES* result = mysql_store_result(conn);
    if (result == nullptr) {
        response["code"] = 3002;
        response["message"] = mysql_error(conn);
        return response;
    }

    // 构建返回json数组，存放多条图片记录
    json list = json::array();

    // 循环读取每一行查询结果
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)) != nullptr) {
        json item;

        // 判空保护，数据库字段为NULL时row[x]为nullptr，防止stoi崩溃
        item["id"] = row[0] ? std::stoi(row[0]) : 0;
        item["channel_id"] = row[1] ? std::stoi(row[1]) : 0;
        item["image_name"] = row[2] ? row[2] : "";
        item["image_path"] = row[3] ? row[3] : "";
        item["capture_time"] = row[4] ? row[4] : "";
        item["create_time"] = row[5] ? row[5] : "";

        // exception_id数据库允许NULL，json直接赋值null
        if (row[6]) {
            item["exception_id"] = std::stoi(row[6]);
        } else {
            item["exception_id"] = nullptr;
        }

        list.push_back(item);
    }

    // 释放MySQL结果集内存，避免内存泄漏，必须调用
    mysql_free_result(result);

    // 组装成功响应：图片记录数组 + 分页参数
    response["code"] = 0;
    response["message"] = "success";
    response["data"]["list"] = list;
    response["data"]["page"] = page;
    response["data"]["page_size"] = pageSize;
    response["data"]["total_count"] = queryImageCount(startTime, endTime, channelId, scope);

    return response;
}

json ImageService::handleInsertImage(const json& request)//插入图片
{
    json response;
    response["type"] = "INSERT_IMAGE_RESP";
    response["request_id"] = request.value("request_id", 0);
    if (!request.contains("data")) {
        response["code"] = 1001;
        response["message"] = "missing data";
        return response;
    }

    const json& data = request["data"];

    int channelId = data.value("channel_id", 0);
    std::string imageName = data.value("image_name", "");
    std::string imagePath = data.value("image_path", "");
    std::string captureTime = data.value("capture_time", "");
    std::string createTime = data.value("create_time", "");

    if (channelId <= 0 || imageName.empty() || imagePath.empty() || captureTime.empty()) {
        response["code"] = 1002;
        response["message"] = "invalid image data";
        return response;
    }

    if (createTime.empty()) {
        createTime = captureTime;
    }

    MYSQL* conn = m_db->handle();

    char escName[512] = {0};
    char escPath[1024] = {0};
    char escCaptureTime[64] = {0};
    char escCreateTime[64] = {0};

    mysql_real_escape_string(conn, escName, imageName.c_str(), imageName.size());
    mysql_real_escape_string(conn, escPath, imagePath.c_str(), imagePath.size());
    mysql_real_escape_string(conn, escCaptureTime, captureTime.c_str(), captureTime.size());
    mysql_real_escape_string(conn, escCreateTime, createTime.c_str(), createTime.size());

    std::string exceptionSql = "NULL";
    if (data.contains("exception_id") && !data["exception_id"].is_null()) {
        exceptionSql = std::to_string(data["exception_id"].get<int>());
    }

    std::string sql =
        "INSERT INTO feature_image "
        "(channel_id, image_name, image_path, capture_time, create_time, exception_id) "
        "VALUES (" +
        std::to_string(channelId) + ", '" +
        std::string(escName) + "', '" +
        std::string(escPath) + "', '" +
        std::string(escCaptureTime) + "', '" +
        std::string(escCreateTime) + "', " +
        exceptionSql + ")";

    if (mysql_query(conn, sql.c_str()) != 0) {
        response["code"] = 3001;
        response["message"] = mysql_error(conn);
        return response;
    }

    response["code"] = 0;
    response["message"] = "success";
    response["data"]["id"] = static_cast<int>(mysql_insert_id(conn));

    return response;
}
json ImageService::handleDeleteImages(const json& request)
{
    json response;
    response["type"] = "DELETE_IMAGES_RESP";
    response["request_id"] = request.value("request_id", 0);
    if (!request.contains("data")) {
        response["code"] = 1001;
        response["message"] = "missing data";
        return response;
    }

    const json& data = request["data"];

    if (!data.contains("image_ids") || !data["image_ids"].is_array()) {
        response["code"] = 1002;
        response["message"] = "missing image_ids";
        return response;
    }

    const json& ids = data["image_ids"];
    if (ids.empty()) {
        response["code"] = 1003;
        response["message"] = "image_ids empty";
        return response;
    }

    std::string idList;
    for (const auto& value : ids) {
        int id = value.get<int>();
        if (id <= 0) {
            continue;
        }

        if (!idList.empty()) {
            idList += ",";
        }
        idList += std::to_string(id);
    }

    if (idList.empty()) {
        response["code"] = 1004;
        response["message"] = "no valid image id";
        return response;
    }

    std::string sql =
        "DELETE FROM feature_image WHERE id IN (" + idList + ")";

    MYSQL* conn = m_db->handle();

    if (mysql_query(conn, sql.c_str()) != 0) {
        response["code"] = 3001;
        response["message"] = mysql_error(conn);
        return response;
    }

    response["code"] = 0;
    response["message"] = "success";
    response["data"]["affected_rows"] = static_cast<int>(mysql_affected_rows(conn));

    return response;
}
