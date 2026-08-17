#include "exceptionmodel.h"

#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "util/dbconnectionpool.h"

ExceptionModel::ExceptionModel()
{
}

int ExceptionModel::insertException(const Exception &exception)
{
    if (!exception.isValid()) {
        return -1;
    }

    DbConnectionGuard guard = DbConnectionPool::instance()->acquire();
    QSqlDatabase db = guard.database();
    if (!db.isOpen()) {
        qDebug() << "database is not open, skip exception record:" << exception.relatedVideoPath();
        return -1;
    }

    QSqlQuery query(db);
    query.prepare("INSERT INTO exception_log(channel_id, event_time, related_videopath) VALUES(?, ?, ?)");
    query.addBindValue(exception.channelId());
    query.addBindValue(exception.eventTime().toString("yyyy-MM-dd HH:mm:ss"));
    query.addBindValue(exception.relatedVideoPath());

    if (!query.exec()) {
        qDebug() << "insert exception record failed:" << query.lastError().text();
        return -1;
    }

    const QVariant idValue = query.lastInsertId();
    return idValue.isValid() ? idValue.toInt() : -1;
}
