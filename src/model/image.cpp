#include "image.h"

Image::Image()
    : m_id(0),
      m_channelId(0),
      m_exceptionId(-1)
{
}

Image::Image(int channelId,
             const QString &imageName,
             const QString &imagePath,
             const QDateTime &captureTime,
             const QDateTime &createTime,
             int exceptionId,
             int id)
    : m_id(id),
      m_channelId(channelId),
      m_imageName(imageName),
      m_imagePath(imagePath),
      m_captureTime(captureTime),
      m_createTime(createTime),
      m_exceptionId(exceptionId)
{
}

bool Image::isValid() const
{
    return m_channelId > 0 && !m_imageName.isEmpty() && !m_imagePath.isEmpty() && m_captureTime.isValid();
}

int Image::id() const
{
    return m_id;
}

int Image::channelId() const
{
    return m_channelId;
}

QString Image::imageName() const
{
    return m_imageName;
}

QString Image::imagePath() const
{
    return m_imagePath;
}

QDateTime Image::captureTime() const
{
    return m_captureTime;
}

QDateTime Image::createTime() const
{
    return m_createTime;
}

int Image::exceptionId() const
{
    return m_exceptionId;
}

void Image::setExceptionId(int exceptionId)
{
    m_exceptionId = exceptionId;
}
