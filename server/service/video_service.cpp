#include "video_service.h"

#include <mysql/mysql.h>

#include <iostream>
#include <string>

/**
 * @brief 视频记录查询服务构造函数
 * @param db 数据库连接对象指针，来自数据库连接池
 */
VideoService::VideoService(DbConnection* db)
    : m_db(db)
{
}

std::string VideoService::escapeString(const std::string& value)
{
    MYSQL* conn = m_db->handle();
    std::string escaped;
    escaped.resize(value.size() * 2 + 1);
    const unsigned long length =
        mysql_real_escape_string(conn, escaped.data(), value.c_str(), value.size());
    escaped.resize(length);
    return escaped;
}

int VideoService::queryVideoCount(const std::string& startTime,
                                  const std::string& endTime,
                                  int channelId,
                                  const std::string& scope)
{
    std::string sql =
        "SELECT COUNT(*) "
        "FROM video "
        "WHERE start_time >= '" + startTime + "' "
        "AND start_time <= '" + endTime + "' ";

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
 * @brief 处理查询视频录像列表业务接口
 * @param request 客户端json请求报文
 * 请求格式示例：
 * {
 *   "type":"QUERY_VIDEO_LIST_REQ",
 *   "data":{
 *       "scope":"normal",   // normal正常视频 / anomaly异常视频
 *       "date":"2026‑08‑17",
 *       "channel_id":0,
 *       "page":1,
 *       "page_size":8
 *   }
 * }
 * @return json 响应报文，返回分页视频数组；携带错误码、提示信息
 * 错误码约定：
 *      0      : 查询成功
 *      1001   : 请求缺少data字段
 *      1002   : date日期参数为空
 *      30xx   : mysql数据库执行异常
 */
json VideoService::handleQueryVideoList(const json& request)
{
    json response;
    // 设置消息类型：查询视频列表响应
    response["type"] = "QUERY_VIDEO_LIST_RESP";
    response["request_id"] = request.value("request_id", 0);
    // 校验请求报文是否包含data业务字段
    if (!request.contains("data")) {
        response["code"] = 1001;
        response["message"] = "missing data";
        return response;
    }

    const json& data = request["data"];

    // 取出请求参数，value第二个参数为取不到时的默认值
    std::string scope = data.value("scope", "normal");   // 筛选范围 normal正常 / anomaly异常
    std::string date = data.value("date", "");           // 查询日期，格式 yyyy‑MM‑dd
    int channelId = data.value("channel_id", 0);        // 通道id，0代表不做通道过滤
    int page = data.value("page", 1);                   // 当前页码，默认第1页
    int pageSize = data.value("page_size", 8);          // 每页条数，默认8条

    // 日期参数不能为空，是查询的必要条件
    if (date.empty()) {
        response["code"] = 1002;
        response["message"] = "date empty";
        return response;
    }

    // 页码防护：页码不能小于1，小于1强制修正为第一页
    if (page < 1) {
        page = 1;
    }

    // 每页条数安全限制：最小1，最大100，防止一次性查询海量数据
    if (pageSize < 1 || pageSize > 100) {
        pageSize = 8;
    }

    // 计算分页偏移量 offset = (页码‑1)*每页数量
    int offset = (page - 1) * pageSize;

    // 拼接时间范围：传入日期的00:00:00 ~ 23:59:59，查询整天的数据
    std::string startTime = date + " 00:00:00";
    std::string endTime = date + " 23:59:59";

    // 基础SQL语句：查询video表的指定字段
    std::string sql =
        "SELECT id, channel_id, video_name, video_path, start_time, end_time, create_time, exception_id "
        "FROM video "
        "WHERE start_time >= '" + startTime + "' "
        "AND start_time <= '" + endTime + "' ";

    // 如果channelId>0，追加条件：只查询指定摄像头通道
    if (channelId > 0) {
        sql += "AND channel_id = " + std::to_string(channelId) + " ";
    }

    // scope筛选逻辑
    // normal：正常视频，exception_id为NULL；anomaly：异常视频，exception_id不为NULL
    if (scope == "normal") {
        sql += "AND exception_id IS NULL ";
    } else if (scope == "anomaly") {
        sql += "AND exception_id IS NOT NULL ";
    }

    // 按录制时间倒序，最新的排在最前面；拼接分页 LIMIT offset,pageSize
    sql +=
        "ORDER BY start_time DESC "
        "LIMIT " + std::to_string(offset) + "," + std::to_string(pageSize);

    // 获取数据库原生连接句柄
    MYSQL* conn = m_db->handle();

    // 执行SQL查询，非0代表执行出错
    if (mysql_query(conn, sql.c_str()) != 0) {
        response["code"] = 3001;
        response["message"] = mysql_error(conn);
        return response;
    }

    // 将查询结果全部拉取到本地内存
    MYSQL_RES* result = mysql_store_result(conn);
    if (result == nullptr) {
        response["code"] = 3002;
        response["message"] = mysql_error(conn);
        return response;
    }

    // 构造返回的json数组，存放多条视频记录
    json list = json::array();

    // 循环读取每一行查询结果
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)) != nullptr) {
        json item;
        // row[n] 为nullptr代表数据库字段为NULL，做判空保护，避免std::stoi崩溃
        item["id"] = row[0] ? std::stoi(row[0]) : 0;
        item["channel_id"] = row[1] ? std::stoi(row[1]) : 0;
        item["video_name"] = row[2] ? row[2] : "";
        item["video_path"] = row[3] ? row[3] : "";
        item["start_time"] = row[4] ? row[4] : "";
        item["end_time"] = row[5] ? row[5] : "";
        item["create_time"] = row[6] ? row[6] : "";

        // exception_id允许数据库为NULL，json赋值为null
        if (row[7]) {
            item["exception_id"] = std::stoi(row[7]);
        } else {
            item["exception_id"] = nullptr;
        }

        list.push_back(item);
    }

    // 释放mysql结果集内存，防止内存泄漏，必须调用
    mysql_free_result(result);

    // 组装成功响应数据：视频数组 + 分页信息
    response["code"] = 0;
    response["message"] = "success";
    response["data"]["list"] = list;
    response["data"]["page"] = page;
    response["data"]["page_size"] = pageSize;
    response["data"]["total_count"] = queryVideoCount(startTime, endTime, channelId, scope);

    return response;
}

json VideoService::handleFindRelatedVideo(const json& request)
{
    json response;
    response["type"] = "FIND_RELATED_VIDEO_RESP";
    response["request_id"] = request.value("request_id", 0);

    if (!request.contains("data")) {
        response["code"] = 1001;
        response["message"] = "missing data";
        response["data"]["record"] = json::object();
        return response;
    }

    const json& data = request["data"];
    const std::string scope = data.value("scope", "normal");
    const int channelId = data.value("channel_id", 0);
    const std::string captureTime = data.value("capture_time", "");

    if (channelId <= 0 || captureTime.empty()) {
        response["code"] = 1002;
        response["message"] = "invalid channel_id or capture_time";
        response["data"]["record"] = json::object();
        return response;
    }

    const std::string escapedCaptureTime = escapeString(captureTime);
    std::string sql =
        "SELECT id, channel_id, video_name, video_path, start_time, end_time "
        "FROM video "
        "WHERE channel_id = " + std::to_string(channelId) + " "
        "AND start_time <= '" + escapedCaptureTime + "' "
        "AND end_time >= '" + escapedCaptureTime + "' ";

    if (scope == "normal") {
        sql += "AND exception_id IS NULL ";
    } else if (scope == "anomaly") {
        sql += "AND exception_id IS NOT NULL ";
    }

    sql += "ORDER BY start_time DESC LIMIT 1";

    MYSQL* conn = m_db->handle();
    if (mysql_query(conn, sql.c_str()) != 0) {
        response["code"] = 3001;
        response["message"] = mysql_error(conn);
        response["data"]["record"] = json::object();
        return response;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (result == nullptr) {
        response["code"] = 3002;
        response["message"] = mysql_error(conn);
        response["data"]["record"] = json::object();
        return response;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    if (row == nullptr) {
        mysql_free_result(result);
        response["code"] = 1003;
        response["message"] = "no related video";
        response["data"]["record"] = json::object();
        return response;
    }

    json record;
    record["id"] = row[0] ? std::stoi(row[0]) : 0;
    record["channel_id"] = row[1] ? std::stoi(row[1]) : 0;
    record["video_name"] = row[2] ? row[2] : "";
    record["video_path"] = row[3] ? row[3] : "";
    record["start_time"] = row[4] ? row[4] : "";
    record["end_time"] = row[5] ? row[5] : "";

    mysql_free_result(result);

    response["code"] = 0;
    response["message"] = "success";
    response["data"]["record"] = record;
    return response;
}

bool VideoService::insertVideoRecord(const json& videoData,
                                     int exceptionId,
                                     int& videoId,
                                     std::string& errorMessage)
{
    const int channelId = videoData.value("channel_id", 0);
    const std::string videoName = videoData.value("video_name", "");
    const std::string videoPath = videoData.value("video_path", "");
    const std::string startTime = videoData.value("start_time", "");
    const std::string endTime = videoData.value("end_time", "");
    std::string createTime = videoData.value("create_time", "");

    if (channelId <= 0 || videoName.empty() || videoPath.empty() || startTime.empty() || endTime.empty()) {
        errorMessage = "invalid video data";
        return false;
    }

    if (createTime.empty()) {
        createTime = startTime;
    }

    int finalExceptionId = exceptionId;
    if (finalExceptionId <= 0 && videoData.contains("exception_id") && !videoData["exception_id"].is_null()) {
        finalExceptionId = videoData["exception_id"].get<int>();
    }

    const std::string exceptionSql = finalExceptionId > 0 ? std::to_string(finalExceptionId) : "NULL";
    const std::string sql =
        "INSERT INTO video "
        "(channel_id, video_name, video_path, start_time, end_time, create_time, exception_id) "
        "VALUES (" +
        std::to_string(channelId) + ", '" +
        escapeString(videoName) + "', '" +
        escapeString(videoPath) + "', '" +
        escapeString(startTime) + "', '" +
        escapeString(endTime) + "', '" +
        escapeString(createTime) + "', " +
        exceptionSql + ")";

    MYSQL* conn = m_db->handle();
    if (mysql_query(conn, sql.c_str()) != 0) {
        errorMessage = mysql_error(conn);
        return false;
    }

    videoId = static_cast<int>(mysql_insert_id(conn));
    return true;
}

bool VideoService::insertImageRecord(const json& imageData,
                                     int exceptionId,
                                     int& imageId,
                                     std::string& errorMessage)
{
    const int channelId = imageData.value("channel_id", 0);
    const std::string imageName = imageData.value("image_name", "");
    const std::string imagePath = imageData.value("image_path", "");
    const std::string captureTime = imageData.value("capture_time", "");
    std::string createTime = imageData.value("create_time", "");

    if (channelId <= 0 || imageName.empty() || imagePath.empty() || captureTime.empty()) {
        errorMessage = "invalid image data";
        return false;
    }

    if (createTime.empty()) {
        createTime = captureTime;
    }

    const std::string sql =
        "INSERT INTO feature_image "
        "(channel_id, image_name, image_path, capture_time, create_time, exception_id) "
        "VALUES (" +
        std::to_string(channelId) + ", '" +
        escapeString(imageName) + "', '" +
        escapeString(imagePath) + "', '" +
        escapeString(captureTime) + "', '" +
        escapeString(createTime) + "', " +
        std::to_string(exceptionId) + ")";

    MYSQL* conn = m_db->handle();
    if (mysql_query(conn, sql.c_str()) != 0) {
        errorMessage = mysql_error(conn);
        return false;
    }

    imageId = static_cast<int>(mysql_insert_id(conn));
    return true;
}

int VideoService::insertExceptionRecord(const json& exceptionData, std::string& errorMessage)
{
    const int channelId = exceptionData.value("channel_id", 0);
    const std::string eventTime = exceptionData.value("event_time", "");
    const std::string relatedVideoPath = exceptionData.value("related_videopath", "");

    if (channelId <= 0 || eventTime.empty() || relatedVideoPath.empty()) {
        errorMessage = "invalid exception data";
        return -1;
    }

    const std::string sql =
        "INSERT INTO exception_log "
        "(channel_id, event_time, related_videopath) "
        "VALUES (" +
        std::to_string(channelId) + ", '" +
        escapeString(eventTime) + "', '" +
        escapeString(relatedVideoPath) + "')";

    MYSQL* conn = m_db->handle();
    if (mysql_query(conn, sql.c_str()) != 0) {
        errorMessage = mysql_error(conn);
        return -1;
    }

    return static_cast<int>(mysql_insert_id(conn));
}

json VideoService::handleInsertVideo(const json& request)
{
    json response;
    response["type"] = "INSERT_VIDEO_RESP";
    response["request_id"] = request.value("request_id", 0);

    if (!request.contains("data")) {
        response["code"] = 1001;
        response["message"] = "missing data";
        return response;
    }

    int videoId = 0;
    std::string errorMessage;
    if (!insertVideoRecord(request["data"], -1, videoId, errorMessage)) {
        response["code"] = 3001;
        response["message"] = errorMessage;
        return response;
    }

    response["code"] = 0;
    response["message"] = "success";
    response["data"]["id"] = videoId;
    return response;
}

json VideoService::handleInsertAnomalySession(const json& request)
{
    json response;
    response["type"] = "INSERT_ANOMALY_SESSION_RESP";
    response["request_id"] = request.value("request_id", 0);
    if (!request.contains("data")) {
        response["code"] = 1001;
        response["message"] = "missing data";
        return response;
    }

    const json& data = request["data"];
    if (!data.contains("exception") || !data.contains("video")
        || !data.contains("images") || !data["images"].is_array()) {
        response["code"] = 1002;
        response["message"] = "missing anomaly session data";
        return response;
    }

    MYSQL* conn = m_db->handle();
    std::string errorMessage;

    if (mysql_query(conn, "START TRANSACTION") != 0) {
        response["code"] = 3001;
        response["message"] = mysql_error(conn);
        return response;
    }

    const int exceptionId = insertExceptionRecord(data["exception"], errorMessage);
    if (exceptionId <= 0) {
        mysql_query(conn, "ROLLBACK");
        response["code"] = 3002;
        response["message"] = errorMessage;
        return response;
    }

    int videoId = 0;
    if (!insertVideoRecord(data["video"], exceptionId, videoId, errorMessage)) {
        mysql_query(conn, "ROLLBACK");
        response["code"] = 3003;
        response["message"] = errorMessage;
        return response;
    }

    json imageIds = json::array();
    for (const auto& imageData : data["images"]) {
        int imageId = 0;
        if (!insertImageRecord(imageData, exceptionId, imageId, errorMessage)) {
            mysql_query(conn, "ROLLBACK");
            response["code"] = 3004;
            response["message"] = errorMessage;
            return response;
        }
        imageIds.push_back(imageId);
    }

    if (mysql_query(conn, "COMMIT") != 0) {
        mysql_query(conn, "ROLLBACK");
        response["code"] = 3005;
        response["message"] = mysql_error(conn);
        return response;
    }

    response["code"] = 0;
    response["message"] = "success";
    response["data"]["exception_id"] = exceptionId;
    response["data"]["video_id"] = videoId;
    response["data"]["image_ids"] = imageIds;
    return response;
}
