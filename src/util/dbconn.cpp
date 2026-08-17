#include "dbconn.h"

#include "appsettings.h"

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
    const DatabaseSettings settings = AppSettings::loadDatabaseSettings();

    db = QSqlDatabase::addDatabase(settings.driver);
    db.setHostName(settings.host);
    db.setPort(settings.port);
    db.setUserName(settings.userName);
    db.setPassword(settings.password);
    db.setDatabaseName(settings.databaseName);

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
    qDebug() << "release database connection";
    if (db.isOpen()) {
        db.close();
    }
    db = QSqlDatabase();
    QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
}
