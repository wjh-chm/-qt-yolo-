#ifndef VIDEOMODEL_H
#define VIDEOMODEL_H

#include "video.h"

#include <QDate>
#include <QList>

class VideoModel
{
public:
    struct VideoRecord {
        int id = 0;
        int channelId = 0;
        QString videoName;
        QString filePath;
        QDateTime startTime;
        QDateTime endTime;
        int exceptionId = -1;
    };

    VideoModel();

    bool insertVideo(const Video &video);
    int countByDate(const QDateTime &startTime,
                    const QDateTime &endTime,
                    int channelId,
                    bool anomalyOnly) const;
    QList<VideoRecord> queryPageByDate(const QDateTime &startTime,
                                       const QDateTime &endTime,
                                       int channelId,
                                       bool anomalyOnly,
                                       int offset,
                                       int limit) const;
    QDate latestDate(bool anomalyOnly) const;
    VideoRecord findCoveringVideo(int channelId,
                                  const QDateTime &time,
                                  bool anomalyOnly) const;
};

#endif // VIDEOMODEL_H
