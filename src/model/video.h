#ifndef VIDEO_H
#define VIDEO_H

#include <QDateTime>
#include <QMetaType>
#include <QString>

class Video
{
public:
    Video();
    Video(int channelId,
          const QString &videoName,
          const QString &filePath,
          const QDateTime &startTime,
          const QDateTime &endTime = QDateTime(),
          int exceptionId = -1);

    bool isValid() const;
    int channelId() const;
    QString videoName() const;
    QString filePath() const;
    QDateTime startTime() const;
    QDateTime endTime() const;
    int exceptionId() const;

    void setExceptionId(int exceptionId);

private:
    int m_channelId;
    QString m_videoName;
    QString m_filePath;
    QDateTime m_startTime;
    QDateTime m_endTime;
    int m_exceptionId;
};

Q_DECLARE_METATYPE(Video)

#endif // VIDEO_H
