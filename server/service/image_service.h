#pragma once

#include "db/db_connection.h"
#include "protocol/protocol.h"

#include <string>

class ImageService {
public:
    explicit ImageService(DbConnection* db);

    json handleQueryImageList(const json& request);
    json handleInsertImage(const json& request);
    json handleDeleteImages(const json& request);

private:
    int queryImageCount(const std::string& startTime,
                        const std::string& endTime,
                        int channelId,
                        const std::string& scope);

private:
    DbConnection* m_db = nullptr;
};
