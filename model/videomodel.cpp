#include "videomodel.h"

#include <QDateTime>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

#include "../util/dbconn.h"

VideoModel::VideoModel()
{
}

bool VideoModel::insertVideo(const Video &video)
{
    // 忽略不完整的视频记录，调用方可以直接透传 stop 的返回值。
    if (!video.isValid()) {
        return false;
    }

    QSqlDatabase db = DbConn::getInstence()->getDb();
    if (!db.isOpen()) {
        // 即使数据库不可用，预览功能也应继续工作，因此这里只记录失败。
        qDebug() << "database is not open, skip video record:" << video.filePath();
        return false;
    }

    QSqlQuery query(db);
    query.prepare(
        "INSERT INTO video(channel_id, video_path, video_name, start_time, end_time, create_time) "
        "VALUES(?, ?, ?, ?, ?, ?)");
    query.addBindValue(video.channelId());
    query.addBindValue(video.filePath());
    query.addBindValue(video.videoName());
    query.addBindValue(video.startTime().toString("yyyy-MM-dd HH:mm:ss"));
    query.addBindValue(video.endTime().toString("yyyy-MM-dd HH:mm:ss"));
    query.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));

    if (!query.exec()) {
        // 写库失败在这里统一记录，界面层不需要处理 SQL 细节。
        qDebug() << "insert video record failed:" << query.lastError().text();
        return false;
    }
    return true;
}
