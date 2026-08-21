#include "protocol.h"

#include <arpa/inet.h>
#include <sys/socket.h>

#include <cstring>
#include <iostream>
#include <vector>

/**
 * @brief 接收一条【4字节长度头 + json报文】应用层协议消息
 * 自定义协议格式： | 4字节网络大端长度(netLen) | JSON字符串载荷(body) |
 * @param fd 客户端socket文件描述符
 * @param out 输出，解析完成的json对象
 * @return true 接收并解析成功；false 连接断开/读失败/长度非法/json解析异常
 */
bool Protocol::recvJson(int fd, json& out)
{
    uint32_t netLen = 0;

    // 第一步：读取4字节【应用层协议头】，保存的是后面json报文的字节长度（网络大端）
    if (!recvAll(fd, reinterpret_cast<char*>(&netLen), sizeof(netLen))) {
        return false;
    }

    // 网络字节序(大端) 转换为本机主机字节序，得到json报文实际字节数
    uint32_t bodyLen = ntohl(netLen);
    // 合法性校验：长度不能为0，设置最大上限1MB防止恶意超大包攻击
    if (bodyLen == 0 || bodyLen > 1024 * 1024) {
        std::cout << "invalid body length: " << bodyLen << std::endl;
        return false;
    }

    // 根据协议头解析出来的长度，分配缓冲区，准备接收完整json字符串
    std::vector<char> buffer(bodyLen);

    // 第二步：读取完整json报文载荷
    if (!recvAll(fd, buffer.data(), bodyLen)) {
        return false;
    }

    // 把收到的char数组解析为json对象，捕获解析异常（报文损坏、非法json）
    try {
        out = json::parse(std::string(buffer.begin(), buffer.end()));
    } catch (const std::exception& e) {
        std::cout << "json parse failed: " << e.what() << std::endl;
        return false;
    }

    return true;
}

/**
 * @brief 发送json对象，封装成自定义协议：4字节长度头 + json字符串
 * @param fd 客户端socket fd
 * @param data 待发送的nlohmann::json对象
 * @return true全部发送成功；false send失败，连接异常
 */
bool Protocol::sendJson(int fd, const json& data)
{
    // json.dump() 将json对象序列化为std::string字符串（业务载荷）
    std::string body = data.dump();

    // 获取json字符串字节长度
    uint32_t bodyLen = static_cast<uint32_t>(body.size());
    // 【组装应用层协议头】：主机字节序转网络大端，4字节长度头
    uint32_t netLen = htonl(bodyLen);

    // 先发送4字节协议头（长度字段）
    if (!sendAll(fd, reinterpret_cast<const char*>(&netLen), sizeof(netLen))) {
        return false;
    }

    // 再发送真正的json业务载荷数据
    return sendAll(fd, body.data(), bodyLen);
}

/**
 * @brief 保证读取len个字节，循环recv，解决TCP分片问题
 * 注意：普通recv不一定一次读完想要的数据，TCP是字节流，会分包返回
 * @param fd socket描述符
 * @param buffer 接收数据缓冲区
 * @param len 需要总共读取的字节数量
 * @return true：读完len字节；false：n<=0 对端关闭连接 / 发生错误
 */
bool Protocol::recvAll(int fd, char* buffer, int len)
{
    int received = 0; // 已经收到的字节计数

    // 循环读取，直到累计收到 len 字节
    while (received < len) {
        // 从偏移位置buffer+received继续读剩余字节
        int n = recv(fd, buffer + received, len - received, 0);

        // n <= 0: n=0代表对端close；n=-1代表出错(EAGAIN/EINTR等)，直接返回失败
        if (n <= 0) {
            return false;
        }

        received += n; // 累加本次读到的字节数
    }

    return true;
}

/**
 * @brief 保证完整发送len字节，循环send，解决TCP发送缓冲区满导致send返回部分字节的问题
 * send不一定一次性把全部数据发出去，内核发送缓冲区满的时候，只会发送一部分
 * @param fd socket描述符
 * @param buffer 待发送数据缓冲区
 * @param len 需要发送的总字节数
 * @return true全部发送完成；false发送过程中断/出错
 */
bool Protocol::sendAll(int fd, const char* buffer, int len)
{
    int sent = 0; // 已经成功发送出去的字节计数

    // 循环发送，直到累计发送 len 字节
    while (sent < len) {
        // 从buffer+sent偏移处发送剩余未发完的数据
        int n = send(fd, buffer + sent, len - sent, 0);

        // n <=0 发送失败，连接断开或者异常
        if (n <= 0) {
            return false;
        }

        sent += n; // 累加本次发送成功字节
    }

    return true;
}