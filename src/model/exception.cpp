#include "exception.h"

Exception::Exception()
    : m_id(0),
      m_channelId(0)
{
}

Exception::Exception(int channelId,
                     const QDateTime &eventTime,
                     const QString &relatedVideoPath,
                     int id)
    : m_id(id),
      m_channelId(channelId),
      m_eventTime(eventTime),
      m_relatedVideoPath(relatedVideoPath)
{
}

bool Exception::isValid() const
{
    return m_channelId > 0 && m_eventTime.isValid() && !m_relatedVideoPath.isEmpty();
}

int Exception::id() const
{
    return m_id;
}

int Exception::channelId() const
{
    return m_channelId;
}

QDateTime Exception::eventTime() const
{
    return m_eventTime;
}

QString Exception::relatedVideoPath() const
{
    return m_relatedVideoPath;
}
