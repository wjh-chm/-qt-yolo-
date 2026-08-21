#include "auth_service.h"

#include <mysql/mysql.h>

#include <iostream>
#include <openssl/md5.h>

#include <iomanip>
#include <sstream>
#include <chrono>
#include <random>

/**
 * @brief 构造函数，传入数据库连接对象
 * @param db 外部传入的数据库连接指针（连接池获取的DbConnection）
 */
AuthService::AuthService(DbConnection* db)
    : m_db(db)
{
}
static std::string md5(const std::string& text)
{
    unsigned char digest[MD5_DIGEST_LENGTH];

    MD5(
        reinterpret_cast<const unsigned char*>(text.c_str()),
        text.size(),
        digest
    );

    std::ostringstream oss;

    for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
        oss << std::hex
            << std::setw(2)
            << std::setfill('0')
            << static_cast<int>(digest[i]);
    }

    return oss.str();
}

static std::string generateToken(int userId)
{
    const auto now = std::chrono::system_clock::now();
    const auto timestamp =
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<int> dist(100000, 999999);

    std::ostringstream oss;
    oss << userId << "_" << timestamp << "_" << dist(gen);
    return oss.str();
}

/**
 * @brief 处理登录业务请求
 * @param request 客户端上传的json请求报文，协议格式：{"type":"LOGIN_REQ","data":{"username":"xxx","password":"xxx"}}
 * @return json 登录响应报文，包含错误码、提示信息、成功后返回user_id与token
 * 错误码约定：
 *      0      : 登录成功
 *      1001   : 请求缺少data字段
 *      1002   : 用户名或者密码为空
 *      2001   : 用户名密码校验失败
 *      30xx   : mysql数据库相关错误
 */
json AuthService::handleLogin(const json& request)
{
    json response;
    // 设置消息类型，对应协议：登录响应
    response["type"] = "LOGIN_RESP";
    response["request_id"] = request.value("request_id", 0);

    // 校验请求报文，判断是否存在data字段
    if (!request.contains("data")) {
        response["code"] = 1001;
        response["message"] = "missing data";
        return response;
    }

    // 从json中取出用户名、密码；value第二个参数为取不到时的默认空字符串

    std::string username = request["data"].value("username", "");
    std::string password = request["data"].value("password", "");
    std::string passwordMd5 = md5(password);

    // 参数校验：用户名、密码不能为空
    if (username.empty() || password.empty()) {
        response["code"] = 1002;
        response["message"] = "username or password empty";
        return response;
    }

    // 获取数据库原生MYSQL连接句柄
    MYSQL* conn = m_db->handle();

    char escUser[256] = {0};
    char escPass[256] = {0};

    /**
     * mysql_real_escape_string：SQL特殊字符转义
     * 防止SQL注入攻击，例如用户输入带单引号 ' 的用户名，破坏SQL语句语法
     * conn：数据库连接；dst输出缓冲区；src原始字符串；length原始字符串长度
     */
    mysql_real_escape_string(conn, escUser, username.c_str(), username.size());
    mysql_real_escape_string(conn, escPass, passwordMd5.c_str(), passwordMd5.size());

    // 拼接查询SQL：根据用户名+密码查询admin_user表，最多返回1条记录
    std::string sql =
        "SELECT id FROM admin_user WHERE username='" + std::string(escUser) +
        "' AND password='" + std::string(escPass) + "' LIMIT 1";

    // 执行SQL语句，mysql_query返回非0代表执行出错
    if (mysql_query(conn, sql.c_str()) != 0) {
        response["code"] = 3001;
        // mysql_error获取数据库的错误描述文本
        response["message"] = mysql_error(conn);
        return response;
    }

    // mysql_store_result：一次性把查询全部结果读到本地内存
    MYSQL_RES* result = mysql_store_result(conn);
    if (result == nullptr) {
        response["code"] = 3002;
        response["message"] = mysql_error(conn);
        return response;
    }

    // 读取一行结果数据
    MYSQL_ROW row = mysql_fetch_row(result);

    // row == nullptr：没有查到匹配记录 → 用户名或密码错误
    if (row == nullptr) {
        // 必须释放结果集，防止内存泄漏
        mysql_free_result(result);
        response["code"] = 2001;
        response["message"] = "username or password error";
        return response;
    }

    // row[0]对应SQL查询出来的id字段，字符串转int得到用户id
    int userId = std::stoi(row[0]);
    // 使用完毕，释放mysql结果集内存，重要！不释放造成内存泄漏
    mysql_free_result(result);

    // 登录成功，填充返回数据
    response["code"] = 0;
    response["message"] = "login success";
    response["data"]["user_id"] = userId;
    response["data"]["token"] = generateToken(userId);

    return response;
}
