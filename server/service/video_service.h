#pragma once

#include "db/db_connection.h"
#include "protocol/protocol.h"

#include <string>

class VideoService {
public:
    explicit VideoService(DbConnection* db);

    json handleQueryVideoList(const json& request);
    json handleFindRelatedVideo(const json& request);
    json handleInsertVideo(const json& request);
    json handleInsertAnomalySession(const json& request);

private:
    int queryVideoCount(const std::string& startTime,
                        const std::string& endTime,
                        int channelId,
                        const std::string& scope);
    std::string escapeString(const std::string& value);
    bool insertVideoRecord(const json& videoData, int exceptionId, int& videoId, std::string& errorMessage);
    bool insertImageRecord(const json& imageData, int exceptionId, int& imageId, std::string& errorMessage);
    int insertExceptionRecord(const json& exceptionData, std::string& errorMessage);

    DbConnection* m_db = nullptr;
};
