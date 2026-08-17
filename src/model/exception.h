#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <QDateTime>
#include <QMetaType>
#include <QString>

class Exception
{
public:
    Exception();
    Exception(int channelId,
              const QDateTime &eventTime,
              const QString &relatedVideoPath,
              int id = 0);

    bool isValid() const;
    int id() const;
    int channelId() const;
    QDateTime eventTime() const;
    QString relatedVideoPath() const;

private:
    int m_id;
    int m_channelId;
    QDateTime m_eventTime;
    QString m_relatedVideoPath;
};

Q_DECLARE_METATYPE(Exception)

#endif // EXCEPTION_H
