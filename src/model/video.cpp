#include "video.h"

Video::Video()
    : m_channelId(0),
      m_exceptionId(-1)
{
}

Video::Video(int channelId,
             const QString &videoName,
             const QString &filePath,
             const QDateTime &startTime,
             const QDateTime &endTime,
             int exceptionId)
    : m_channelId(channelId),
      m_videoName(videoName),
      m_filePath(filePath),
      m_startTime(startTime),
      m_endTime(endTime),
      m_exceptionId(exceptionId)
{
}

bool Video::isValid() const
{
    return m_channelId > 0 && !m_videoName.isEmpty() && !m_filePath.isEmpty() && m_startTime.isValid();
}

int Video::channelId() const
{
    return m_channelId;
}

QString Video::videoName() const
{
    return m_videoName;
}

QString Video::filePath() const
{
    return m_filePath;
}

QDateTime Video::startTime() const
{
    return m_startTime;
}

QDateTime Video::endTime() const
{
    return m_endTime;
}

int Video::exceptionId() const
{
    return m_exceptionId;
}

void Video::setExceptionId(int exceptionId)
{
    m_exceptionId = exceptionId;
}
