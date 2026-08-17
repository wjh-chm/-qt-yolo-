#include "videomodel.h"

#include <QDateTime>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "util/dbconnectionpool.h"

VideoModel::VideoModel()
{
}

bool VideoModel::insertVideo(const Video &video)
{
    if (!video.isValid()) {
        return false;
    }

    DbConnectionGuard guard = DbConnectionPool::instance()->acquire();
    QSqlDatabase db = guard.database();
    if (!db.isOpen()) {
        qDebug() << "database is not open, skip video record:" << video.filePath();
        return false;
    }

    QSqlQuery query(db);
    query.prepare(
        "INSERT INTO video(channel_id, video_path, video_name, start_time, end_time, create_time, exception_id) "
        "VALUES(?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(video.channelId());
    query.addBindValue(video.filePath());
    query.addBindValue(video.videoName());
    query.addBindValue(video.startTime().toString("yyyy-MM-dd HH:mm:ss"));
    query.addBindValue(video.endTime().toString("yyyy-MM-dd HH:mm:ss"));
    query.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    query.addBindValue(video.exceptionId() > 0 ? QVariant(video.exceptionId()) : QVariant());

    if (!query.exec()) {
        qDebug() << "insert video record failed:" << query.lastError().text();
        return false;
    }
    return true;
}

int VideoModel::countByDate(const QDateTime &startTime,
                            const QDateTime &endTime,
                            int channelId,
                            bool anomalyOnly) const
{
    DbConnectionGuard guard = DbConnectionPool::instance()->acquire();
    QSqlDatabase db = guard.database();
    if (!db.isOpen()) {
        return 0;
    }

    QSqlQuery query(db);
    QString sql = "SELECT COUNT(*) FROM video WHERE start_time >= ? AND start_time < ? AND ";
    sql += anomalyOnly ? "exception_id IS NOT NULL" : "exception_id IS NULL";
    if (channelId > 0) {
        sql += " AND channel_id = ?";
    }

    query.prepare(sql);
    query.addBindValue(startTime.toString("yyyy-MM-dd HH:mm:ss"));
    query.addBindValue(endTime.toString("yyyy-MM-dd HH:mm:ss"));
    if (channelId > 0) {
        query.addBindValue(channelId);
    }

    if (!query.exec() || !query.next()) {
        qDebug() << "query video count failed:" << query.lastError().text();
        return 0;
    }

    return query.value(0).toInt();
}

QList<VideoModel::VideoRecord> VideoModel::queryPageByDate(const QDateTime &startTime,
                                                           const QDateTime &endTime,
                                                           int channelId,
                                                           bool anomalyOnly,
                                                           int offset,
                                                           int limit) const
{
    QList<VideoRecord> records;

    DbConnectionGuard guard = DbConnectionPool::instance()->acquire();
    QSqlDatabase db = guard.database();
    if (!db.isOpen()) {
        return records;
    }

    QSqlQuery query(db);
    QString sql = "SELECT id, channel_id, video_name, start_time, end_time, video_path, exception_id "
                  "FROM video WHERE start_time >= ? AND start_time < ? AND ";
    sql += anomalyOnly ? "exception_id IS NOT NULL" : "exception_id IS NULL";
    if (channelId > 0) {
        sql += " AND channel_id = ?";
    }
    sql += " ORDER BY start_time DESC LIMIT ?, ?";

    query.prepare(sql);
    query.addBindValue(startTime.toString("yyyy-MM-dd HH:mm:ss"));
    query.addBindValue(endTime.toString("yyyy-MM-dd HH:mm:ss"));
    if (channelId > 0) {
        query.addBindValue(channelId);
    }
    query.addBindValue(offset);
    query.addBindValue(limit);

    if (!query.exec()) {
        qDebug() << "query video page failed:" << query.lastError().text();
        return records;
    }

    while (query.next()) {
        VideoRecord record;
        record.id = query.value("id").toInt();
        record.channelId = query.value("channel_id").toInt();
        record.videoName = query.value("video_name").toString();
        record.filePath = query.value("video_path").toString();
        record.startTime = query.value("start_time").toDateTime();
        record.endTime = query.value("end_time").toDateTime();
        record.exceptionId = query.value("exception_id").toInt();
        records.append(record);
    }

    return records;
}

QDate VideoModel::latestDate(bool anomalyOnly) const
{
    DbConnectionGuard guard = DbConnectionPool::instance()->acquire();
    QSqlDatabase db = guard.database();
    if (!db.isOpen()) {
        return QDate();
    }

    QSqlQuery query(db);
    QString sql = "SELECT MAX(start_time) FROM video WHERE ";
    sql += anomalyOnly ? "exception_id IS NOT NULL" : "exception_id IS NULL";
    if (!query.exec(sql) || !query.next()) {
        qDebug() << "query latest video date failed:" << query.lastError().text();
        return QDate();
    }

    const QDateTime latestTime = query.value(0).toDateTime();
    return latestTime.isValid() ? latestTime.date() : QDate();
}

VideoModel::VideoRecord VideoModel::findCoveringVideo(int channelId,
                                                      const QDateTime &time,
                                                      bool anomalyOnly) const
{
    VideoRecord record;
    if (!time.isValid()) {
        return record;
    }

    DbConnectionGuard guard = DbConnectionPool::instance()->acquire();
    QSqlDatabase db = guard.database();
    if (!db.isOpen()) {
        return record;
    }

    QSqlQuery query(db);
    QString sql = "SELECT id, channel_id, video_name, start_time, end_time, video_path, exception_id "
                  "FROM video WHERE channel_id = ? AND start_time <= ? AND end_time >= ? AND ";
    sql += anomalyOnly ? "exception_id IS NOT NULL" : "exception_id IS NULL";
    sql += " ORDER BY start_time DESC LIMIT 1";

    query.prepare(sql);
    query.addBindValue(channelId);
    query.addBindValue(time.toString("yyyy-MM-dd HH:mm:ss"));
    query.addBindValue(time.toString("yyyy-MM-dd HH:mm:ss"));

    if (!query.exec() || !query.next()) {
        qDebug() << "query covering video failed:" << query.lastError().text();
        return record;
    }

    record.id = query.value("id").toInt();
    record.channelId = query.value("channel_id").toInt();
    record.videoName = query.value("video_name").toString();
    record.filePath = query.value("video_path").toString();
    record.startTime = query.value("start_time").toDateTime();
    record.endTime = query.value("end_time").toDateTime();
    record.exceptionId = query.value("exception_id").toInt();
    return record;
}
