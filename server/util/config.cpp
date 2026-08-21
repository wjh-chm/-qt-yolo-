#include "config.h"

#include <fstream>
#include <sstream>

bool Config::load(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '[' || line[0] == '#') {
            continue;
        }

        std::size_t pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }

        std::string key = line.substr(0, pos);
        std::string val = line.substr(pos + 1);
        m_values[key] = val;
    }

    return true;
}

std::string Config::value(const std::string& key) const
{
    auto it = m_values.find(key);
    if (it == m_values.end()) {
        return "";
    }
    return it->second;
}

std::string Config::dbHost() const
{
    return value("host");
}

int Config::dbPort() const
{
    std::string port = value("port");
    if (port.empty()) {
        return 3306;
    }
    return std::stoi(port);
}

std::string Config::dbUser() const
{
    return value("user");
}

std::string Config::dbPassword() const
{
    return value("password");
}

std::string Config::dbName() const
{
    return value("name");
}