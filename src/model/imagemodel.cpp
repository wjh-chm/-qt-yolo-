#include "imagemodel.h"

#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QVariant>

#include "util/dbconnectionpool.h"

ImageModel::ImageModel()
{
}

bool ImageModel::insertImage(const Image &image)
{
    if (!image.isValid()) {
        return false;
    }

    DbConnectionGuard guard = DbConnectionPool::instance()->acquire();
    QSqlDatabase db = guard.database();
    if (!db.isOpen()) {
        qDebug() << "database is not open, skip image record:" << image.imagePath();
        return false;
    }

    QSqlQuery query(db);
    query.prepare(
        "INSERT INTO feature_image(image_name, image_path, channel_id, capture_time, create_time, exception_id) "
        "VALUES(?, ?, ?, ?, ?, ?)");
    query.addBindValue(image.imageName());
    query.addBindValue(image.imagePath());
    query.addBindValue(image.channelId());
    query.addBindValue(image.captureTime().toString("yyyy-MM-dd HH:mm:ss"));
    query.addBindValue(image.createTime().isValid()
                           ? image.createTime().toString("yyyy-MM-dd HH:mm:ss")
                           : image.captureTime().toString("yyyy-MM-dd HH:mm:ss"));
    query.addBindValue(image.exceptionId() > 0 ? QVariant(image.exceptionId()) : QVariant());

    if (query.exec()) {
        return true;
    }

    QSqlQuery fallbackQuery(db);
    fallbackQuery.prepare(
        "INSERT INTO feature_image(image_name, image_path, channel_id, capture_time, exception_id) "
        "VALUES(?, ?, ?, ?, ?)");
    fallbackQuery.addBindValue(image.imageName());
    fallbackQuery.addBindValue(image.imagePath());
    fallbackQuery.addBindValue(image.channelId());
    fallbackQuery.addBindValue(image.captureTime().toString("yyyy-MM-dd HH:mm:ss"));
    fallbackQuery.addBindValue(image.exceptionId() > 0 ? QVariant(image.exceptionId()) : QVariant());

    if (!fallbackQuery.exec()) {
        qDebug() << "insert image record failed:" << fallbackQuery.lastError().text();
        return false;
    }

    return true;
}

bool ImageModel::deleteImagesByIds(const QList<int> &imageIds)
{
    if (imageIds.isEmpty()) {
        return true;
    }

    DbConnectionGuard guard = DbConnectionPool::instance()->acquire();
    QSqlDatabase db = guard.database();
    if (!db.isOpen()) {
        qDebug() << "database is not open, skip delete image records";
        return false;
    }

    if (!db.transaction()) {
        qDebug() << "start delete image transaction failed:" << db.lastError().text();
        return false;
    }

    QSqlQuery query(db);
    query.prepare("DELETE FROM feature_image WHERE id = ?");
    for (int imageId : imageIds) {
        query.bindValue(0, imageId);
        if (!query.exec()) {
            qDebug() << "delete image record failed:" << imageId << query.lastError().text();
            db.rollback();
            return false;
        }
    }

    if (!db.commit()) {
        qDebug() << "commit delete image transaction failed:" << db.lastError().text();
        db.rollback();
        return false;
    }

    return true;
}

QList<Image> ImageModel::findImagesByIds(const QList<int> &imageIds)
{
    QList<Image> images;
    if (imageIds.isEmpty()) {
        return images;
    }

    DbConnectionGuard guard = DbConnectionPool::instance()->acquire();
    QSqlDatabase db = guard.database();
    if (!db.isOpen()) {
        qDebug() << "database is not open, skip query image records";
        return images;
    }

    QStringList placeholders;
    for (int i = 0; i < imageIds.size(); ++i) {
        placeholders.append("?");
    }

    QSqlQuery query(db);
    query.prepare(QString("SELECT id, channel_id, image_name, image_path, capture_time, create_time, exception_id "
                          "FROM feature_image WHERE id IN (%1)")
                      .arg(placeholders.join(",")));
    for (int imageId : imageIds) {
        query.addBindValue(imageId);
    }

    if (!query.exec()) {
        qDebug() << "query image records failed:" << query.lastError().text();
        return images;
    }

    while (query.next()) {
        images.append(Image(query.value("channel_id").toInt(),
                            query.value("image_name").toString(),
                            query.value("image_path").toString(),
                            query.value("capture_time").toDateTime(),
                            query.value("create_time").toDateTime(),
                            query.value("exception_id").toInt(),
                            query.value("id").toInt()));
    }

    return images;
}

int ImageModel::countByDate(const QDateTime &startTime,
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
    QString sql = "SELECT COUNT(*) FROM feature_image WHERE capture_time >= ? AND capture_time < ? AND ";
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
        qDebug() << "query image count failed:" << query.lastError().text();
        return 0;
    }

    return query.value(0).toInt();
}

QList<Image> ImageModel::queryPageByDate(const QDateTime &startTime,
                                         const QDateTime &endTime,
                                         int channelId,
                                         bool anomalyOnly,
                                         int offset,
                                         int limit) const
{
    QList<Image> images;

    DbConnectionGuard guard = DbConnectionPool::instance()->acquire();
    QSqlDatabase db = guard.database();
    if (!db.isOpen()) {
        return images;
    }

    QSqlQuery query(db);
    QString sql = "SELECT id, channel_id, image_name, capture_time, create_time, image_path, exception_id "
                  "FROM feature_image WHERE capture_time >= ? AND capture_time < ? AND ";
    sql += anomalyOnly ? "exception_id IS NOT NULL" : "exception_id IS NULL";
    if (channelId > 0) {
        sql += " AND channel_id = ?";
    }
    sql += " ORDER BY capture_time DESC LIMIT ?, ?";

    query.prepare(sql);
    query.addBindValue(startTime.toString("yyyy-MM-dd HH:mm:ss"));
    query.addBindValue(endTime.toString("yyyy-MM-dd HH:mm:ss"));
    if (channelId > 0) {
        query.addBindValue(channelId);
    }
    query.addBindValue(offset);
    query.addBindValue(limit);

    if (!query.exec()) {
        qDebug() << "query image page failed:" << query.lastError().text();
        return images;
    }

    while (query.next()) {
        images.append(Image(query.value("channel_id").toInt(),
                            query.value("image_name").toString(),
                            query.value("image_path").toString(),
                            query.value("capture_time").toDateTime(),
                            query.value("create_time").toDateTime(),
                            query.value("exception_id").toInt(),
                            query.value("id").toInt()));
    }

    return images;
}

QDate ImageModel::latestDate(bool anomalyOnly) const
{
    DbConnectionGuard guard = DbConnectionPool::instance()->acquire();
    QSqlDatabase db = guard.database();
    if (!db.isOpen()) {
        return QDate();
    }

    QSqlQuery query(db);
    QString sql = "SELECT MAX(capture_time) FROM feature_image WHERE ";
    sql += anomalyOnly ? "exception_id IS NOT NULL" : "exception_id IS NULL";
    if (!query.exec(sql) || !query.next()) {
        qDebug() << "query latest image date failed:" << query.lastError().text();
        return QDate();
    }

    const QDateTime latestTime = query.value(0).toDateTime();
    return latestTime.isValid() ? latestTime.date() : QDate();
}
