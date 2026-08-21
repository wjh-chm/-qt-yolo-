#pragma once

#include "db/db_connection.h"
#include "protocol/protocol.h"

#include <string>
#include <unordered_map>

class TcpServer {
public:
    explicit TcpServer(DbConnection* db);

    bool start(int port);

private:
    bool isLoginRequest(const std::string& type) const;//判断是不是登录请求，登录请求不需要 token。
    bool isValidToken(const json& request) const;//判断请求里的 token 是否存在于服务端。
    std::string responseTypeFromRequestType(const std::string& requestType) const;
    json makeUnauthorizedResponse(const json& request, const std::string& requestType) const;//生成“未登录/无权限”的响应。
    void saveTokenFromLoginResponse(const json& response);//登录成功后，把 token 保存到服务端内存里。
private:
    DbConnection* m_db = nullptr;

    // token -> user_id
    std::unordered_map<std::string, int> m_tokenUsers;//服务端的登录状态表
};
