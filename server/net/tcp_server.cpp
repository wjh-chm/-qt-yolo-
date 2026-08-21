#include "tcp_server.h"
#include "protocol/protocol.h"
#include "service/auth_service.h"
#include "service/video_service.h"
#include "service/image_service.h"
#include "service/log_service.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>


/**
 * @brief 启动TCP服务端，阻塞循环等待客户端连接
 * @param port 监听端口号
 * @return 成功返回true；socket/bind/listen失败返回false
 */
 TcpServer::TcpServer(DbConnection* db)
    : m_db(db)
{
}
bool TcpServer::start(int port)
{
    // 创建TCP流式socket，AF_INET：IPv4，SOCK_STREAM：TCP
    int listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd < 0) {
        std::cout << "socket create failed" << std::endl;
        return false;
    }

    int opt = 1;
    // 设置套接字选项 SO_REUSEADDR：允许端口立刻复用，服务端重启不会报bind失败（TIME_WAIT问题）
    setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr;
    // 将地址结构体全部置0，避免垃圾字节
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;         // IPv4协议族
    addr.sin_addr.s_addr = INADDR_ANY; // INADDR_ANY：监听本机所有网卡IP(0.0.0.0)
    addr.sin_port = htons(port);       // htons：主机字节序转网络大端字节序

    // bind：将socket fd绑定到指定IP+端口
    if (bind(listenFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cout << "bind failed" << std::endl;
        close(listenFd); // 出错要关闭文件描述符，防止文件句柄泄漏
        return false;
    }

    // listen：把socket转为监听状态；第二个参数是内核未完成连接队列的最大长度
    if (listen(listenFd, 10) < 0) {
        std::cout << "listen failed" << std::endl;
        close(listenFd);
        return false;
    }

    std::cout << "tcp server listen on port " << port << std::endl;

    // 死循环：持续等待新客户端连接
    while (true) {
        sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);

        /**
         * accept 阻塞函数
         * 从内核连接队列取出一条已经完成三次握手的连接
         * 返回新的clientFd：专门用来和这个客户端通信
         * listenFd 永远只负责监听，不做数据收发
        */
        int clientFd = accept(
            listenFd,
            reinterpret_cast<sockaddr*>(&clientAddr),
            &clientLen
        );

        if (clientFd < 0) {
            std::cout << "accept failed" << std::endl;
            continue;
        }

        // inet_ntop：把网络二进制IP，转换成人类可读的点分字符串ip
        char ip[64] = {0};
        inet_ntop(AF_INET, &clientAddr.sin_addr, ip, sizeof(ip));

        std::cout << "client connected: " << ip << std::endl;

        // char buffer[1024] = {0};
        // // recv 阻塞接收客户端数据；flags=0普通读取
        // int n = recv(clientFd, buffer, sizeof(buffer) - 1, 0);

        // if (n > 0) {
        //     buffer[n] = '\0'; // recv不会自动追加字符串结束符，手动补'\0'，防止cout乱码
        //     std::cout << "recv: " << buffer << std::endl;

        //     const char* response = "hello client";
        //     // send向客户端发送应答数据
        //     send(clientFd, response, std::strlen(response), 0);
        // }
        while (true) {
            json request;

            if (!Protocol::recvJson(clientFd, request)) {
                std::cout << "client disconnected" << std::endl;
                break;
            }

            std::cout << "recv json: " << request.dump() << std::endl;

            json response;

            std::string type = request.value("type", "");
            if (!isLoginRequest(type) && !isValidToken(request)) {
                response = makeUnauthorizedResponse(request, type);

                if (!Protocol::sendJson(clientFd, response)) {
                    std::cout << "send response failed" << std::endl;
                    break;
                }

                continue;
            }
            if (type == "LOGIN_REQ") {
                AuthService authService(m_db);
                response = authService.handleLogin(request);
                saveTokenFromLoginResponse(response);
            } else if (type == "QUERY_VIDEO_LIST_REQ") {
                VideoService videoService(m_db);
                response = videoService.handleQueryVideoList(request);
            } else if (type == "FIND_RELATED_VIDEO_REQ") {
                VideoService videoService(m_db);
                response = videoService.handleFindRelatedVideo(request);
            } else if (type == "INSERT_VIDEO_REQ") {
                VideoService videoService(m_db);
                response = videoService.handleInsertVideo(request);
            } else if (type == "INSERT_ANOMALY_SESSION_REQ") {
                VideoService videoService(m_db);
                response = videoService.handleInsertAnomalySession(request);
            }else if (type == "QUERY_IMAGE_LIST_REQ") {
                ImageService imageService(m_db);
                response = imageService.handleQueryImageList(request);
            } else if (type == "INSERT_IMAGE_REQ") {
                ImageService imageService(m_db);
                response = imageService.handleInsertImage(request);
            }else if (type == "DELETE_IMAGES_REQ") {
                ImageService imageService(m_db);
                response = imageService.handleDeleteImages(request);
            }else if (type == "QUERY_LOG_LIST_REQ") {
                LogService logService(m_db);
                response = logService.handleQueryLogList(request);
            } else {
                response["type"] = "UNKNOWN_RESP";
                response["request_id"] = request.value("request_id", 0);
                response["code"] = 1000;
                response["message"] = "unknown request type";
            }

            if (!Protocol::sendJson(clientFd, response)) {
                std::cout << "send response failed" << std::endl;
                break;
            }
        }

        close(clientFd);
    }

    // 注意：上面while(true)死循环，代码永远执行不到这里
    close(listenFd);
    return true;
}
bool TcpServer::isLoginRequest(const std::string& type) const
{
    return type == "LOGIN_REQ";
}
bool TcpServer::isValidToken(const json& request) const
{
    const std::string token = request.value("token", "");
    if (token.empty()) {
        return false;
    }

    return m_tokenUsers.find(token) != m_tokenUsers.end();
}

std::string TcpServer::responseTypeFromRequestType(const std::string& requestType) const
{
    const std::string suffix = "_REQ";

    if (requestType.size() >= suffix.size()
        && requestType.compare(requestType.size() - suffix.size(), suffix.size(), suffix) == 0) {
        return requestType.substr(0, requestType.size() - suffix.size()) + "_RESP";
    }

    return "UNKNOWN_RESP";
}

json TcpServer::makeUnauthorizedResponse(const json& request,
                                         const std::string& requestType) const
{
    json response;

    response["type"] = responseTypeFromRequestType(requestType);
    response["request_id"] = request.value("request_id", 0);
    response["code"] = 401;
    response["message"] = "unauthorized, please login first";

    return response;
}
void TcpServer::saveTokenFromLoginResponse(const json& response)
{
    if (response.value("code", -1) != 0) {
        return;
    }

    if (!response.contains("data")) {
        return;
    }

    const json& data = response["data"];
    const std::string token = data.value("token", "");
    const int userId = data.value("user_id", 0);

    if (!token.empty() && userId > 0) {
        m_tokenUsers[token] = userId;
    }
}
