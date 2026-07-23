#ifndef VIDEO_H
#define VIDEO_H

#include <QDateTime>
#include <QString>

class Video
{
public:
    Video();
    Video(int channelId,
          const QString &videoName,
          const QString &filePath,
          const QDateTime &startTime,
          const QDateTime &endTime = QDateTime());

    bool isValid() const;
    int channelId() const;
    QString videoName() const;
    QString filePath() const;
    QDateTime startTime() const;
    QDateTime endTime() const;

private:
    int m_channelId;
    QString m_videoName;
    QString m_filePath;
    QDateTime m_startTime;
    QDateTime m_endTime;
};

#endif // VIDEO_H
