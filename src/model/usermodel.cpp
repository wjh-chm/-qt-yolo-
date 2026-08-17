#include "usermodel.h"

#include "util/dbconnectionpool.h"
#include "util/dbconn.h"

#include <QDebug>
#include <QSqlDatabase>
#include <QSqlQuery>

// 该模型本身不保存状态，只通过共享数据库连接执行查询。
UserModel::UserModel()
{
}

bool UserModel::queryUserAndPwd(QString &usrname, QString &usrpwd, int &userId)
{
    // 从共享连接中取数据库对象，避免界面层直接编写 SQL。
    DbConnectionGuard guard = DbConnectionPool::instance()->acquire();
    QSqlDatabase db = guard.database();
    if (!db.isOpen()) {
        qDebug() << "database open failed";
        return false;
    }

    // 使用预编译 SQL，统一处理参数绑定和转义。
    QString sql = "select * from admin_user where username=:username and password=:password";
    QSqlQuery query(db);
    query.prepare(sql);

    // 数据库存的是 MD5，因此这里先对输入密码做同样的哈希。
    query.bindValue(":username", usrname);
    query.bindValue(":password", DbConn::getInstence()->getMd5(usrpwd));

    if (query.exec() && query.next()) {
        userId = query.value("id").toInt();
        return true;
    }
    return false;
}

int UserModel::updateLoginTime(int &userId)
{
    // 登录成功后记录最后登录时间，便于后续审计和日志扩展。
    DbConnectionGuard guard = DbConnectionPool::instance()->acquire();
    QSqlDatabase db = guard.database();
    if (!db.isOpen()) {
        qDebug() << "database open failed";
        return -1;
    }

    QString sql = "update admin_user set last_login_time=:logintime where id=:id";
    QSqlQuery query(db);
    query.prepare(sql);
    query.bindValue(":id", userId);
    query.bindValue(":logintime", QDateTime::currentDateTime());
    if (query.exec()) {
        return query.numRowsAffected();
    }
    return -1;
}
