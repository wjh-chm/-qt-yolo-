#include "userservice.h"

UserService::UserService()
{
}

bool UserService::login(QString usrname, QString usrpwd)
{
    int usrId = 0;

    // 只有账号校验和登录时间更新都成功，才算一次完整登录。
    bool result = userModel.queryUserAndPwd(usrname, usrpwd, usrId);
    if (result) {
        int row = userModel.updateLoginTime(usrId);
        if (row > -1) {
            return true;
        }
    }
    return false;
}
