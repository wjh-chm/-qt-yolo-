#include "dbconn.h"

// 懒加载的单例实例。
DbConn *DbConn::s_instence = nullptr;

DbConn *DbConn::getInstence()
{
    if (DbConn::s_instence == nullptr) {
        DbConn::s_instence = new DbConn;
    }
    return DbConn::s_instence;
}

void DbConn::releaseInstence()
{
    if (DbConn::s_instence != nullptr) {
        delete DbConn::s_instence;
        DbConn::s_instence = nullptr;
    }
}

QSqlDatabase DbConn::getDb() const
{
    return db;
}

DbConn::DbConn()
{
    // 当前里程碑版本使用固定配置连接本地 MySQL 库。
    db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName("localhost");
    db.setPort(3306);
    db.setUserName("lili");
    db.setPassword("123456");
    db.setDatabaseName("smart_minitor");

    // 启动时立即尝试连接，业务层只需要根据 isOpen() 决定是否降级。
    if (!db.open()) {
        qDebug() << "database connection failed" << db.lastError().text();
        return;
    }
    qDebug() << "database connection ready";
}

QString DbConn::getMd5(const QString &str)
{
    QByteArray byteArr = str.toUtf8();
    QByteArray md5Arr = QCryptographicHash::hash(byteArr, QCryptographicHash::Md5);
    QString md5Str = md5Arr.toHex();
    return md5Str;
}

DbConn::~DbConn()
{
    // 程序退出时关闭并注销共享连接。
    qDebug() << "release database connection";
    if (db.isOpen()) {
        db.close();
        QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
    }
}
