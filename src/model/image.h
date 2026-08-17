#ifndef IMAGE_H
#define IMAGE_H

#include <QDateTime>
#include <QList>
#include <QMetaType>
#include <QString>

class Image
{
public:
    Image();
    Image(int channelId,
          const QString &imageName,
          const QString &imagePath,
          const QDateTime &captureTime,
          const QDateTime &createTime = QDateTime(),
          int exceptionId = -1,
          int id = 0);

    bool isValid() const;
    int id() const;
    int channelId() const;
    QString imageName() const;
    QString imagePath() const;
    QDateTime captureTime() const;
    QDateTime createTime() const;
    int exceptionId() const;

    void setExceptionId(int exceptionId);

private:
    int m_id;
    int m_channelId;
    QString m_imageName;
    QString m_imagePath;
    QDateTime m_captureTime;
    QDateTime m_createTime;
    int m_exceptionId;
};

using ImageList = QList<Image>;

Q_DECLARE_METATYPE(Image)
Q_DECLARE_METATYPE(ImageList)

#endif // IMAGE_H
