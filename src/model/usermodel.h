#ifndef USERMODEL_H
#define USERMODEL_H

#include <QDateTime>
#include <QString>

// 管理员账号的数据访问封装。
class UserModel
{
public:
    UserModel();

    // 校验用户名和密码，并返回匹配到的用户编号。
    bool queryUserAndPwd(QString &usrname, QString &usrpwd, int &userId);

    // 登录成功后更新用户最近一次登录时间。
    int updateLoginTime(int &userId);
};

#endif // USERMODEL_H
