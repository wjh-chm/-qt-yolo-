#pragma once

#include <string>
#include <unordered_map>

class Config {
public:
    bool load(const std::string& path);

    std::string value(const std::string& key) const;
    std::string dbHost() const;
    int dbPort() const;
    std::string dbUser() const;
    std::string dbPassword() const;
    std::string dbName() const;

private:
    std::unordered_map<std::string, std::string> m_values;
};