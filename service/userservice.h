#ifndef USERSERVICE_H
#define USERSERVICE_H

#include "../model/usermodel.h"

#include <QString>

// 负责协调登录业务流程的服务层。
class UserService
{
public:
    UserService();

    // 完成用户认证，并更新最近登录时间。
    bool login(QString usrname, QString usrpwd);

private:
    UserModel userModel;
};

#endif // USERSERVICE_H
