#pragma once

#include <nlohmann/json.hpp>

#include <string>

using json = nlohmann::json;

class Protocol {
public:
    static bool recvJson(int fd, json& out);
    static bool sendJson(int fd, const json& data);

private:
    static bool recvAll(int fd, char* buffer, int len);
    static bool sendAll(int fd, const char* buffer, int len);
};