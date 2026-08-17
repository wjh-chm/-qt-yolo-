#ifndef DBCONN_H
#define DBCONN_H

#include <QCryptographicHash>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

// 共享 MySQL 连接与哈希工具的单例封装。
class DbConn
{
public:
    static DbConn *getInstence();
    void releaseInstence();

    // 全局只维护一个连接对象，因此禁止拷贝。
    DbConn(const DbConn &) = delete;
    DbConn &operator=(const DbConn &) = delete;

    QSqlDatabase getDb() const;

    // 把密码哈希逻辑集中在这里，避免调用方重复处理加密细节。
    QString getMd5(const QString &str);

private:
    DbConn();
    ~DbConn();

    static DbConn *s_instence;
    QSqlDatabase db;
};

#endif // DBCONN_H
