#include "logmodel.h"

#include "util/dbconnectionpool.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

LogModel::LogModel()
{
}

int LogModel::count(Mode mode) const
{
    DbConnectionGuard guard = DbConnectionPool::instance()->acquire();
    const QSqlDatabase db = guard.database();
    if (!db.isOpen()) {
        return 0;
    }

    QSqlQuery query(db);
    const QString sql = mode == Mode::Exception
                            ? QStringLiteral("SELECT COUNT(*) FROM exception_log")
                            : QStringLiteral("SELECT COUNT(*) FROM operation_log");
    if (!query.exec(sql) || !query.next()) {
        qDebug() << "query log count failed:" << query.lastError().text();
        return 0;
    }

    return query.value(0).toInt();
}

QList<LogModel::Record> LogModel::queryPage(Mode mode, int offset, int limit) const
{
    QList<Record> records;

    DbConnectionGuard guard = DbConnectionPool::instance()->acquire();
    const QSqlDatabase db = guard.database();
    if (!db.isOpen()) {
        return records;
    }

    QSqlQuery query(db);
    if (mode == Mode::Exception) {
        query.prepare("SELECT id, event_time "
                      "FROM exception_log ORDER BY event_time DESC LIMIT ?, ?");
    } else {
        query.prepare("SELECT id, operation_desc, admin_id, operation_time "
                      "FROM operation_log ORDER BY operation_time DESC LIMIT ?, ?");
    }
    query.addBindValue(offset);
    query.addBindValue(limit);

    if (!query.exec()) {
        qDebug() << "query log page failed:" << query.lastError().text();
        return records;
    }

    while (query.next()) {
        Record record;
        record.id = query.value("id").toInt();
        if (mode == Mode::Exception) {
            record.operation = QStringLiteral("异常触发");
            record.operatorId = 1;
            record.operateTime = query.value("event_time").toString();
        } else {
            record.operation = query.value("operation_desc").toString();
            record.operatorId = query.value("admin_id").toInt();
            record.operateTime = query.value("operation_time").toString();
        }
        records.append(record);
    }

    return records;
}
