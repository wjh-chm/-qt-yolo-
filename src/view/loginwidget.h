#ifndef LOGINWIDGET_H
#define LOGINWIDGET_H

#include "verifycodelabel.h"

#include "service/userservice.h"
#include "util/dbconn.h"

#include <QBrush>
#include <QDebug>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPalette>
#include <QPixmap>
#include <QPushButton>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVBoxLayout>
#include <QWidget>

// 登录窗口：先校验验证码，再把账号密码校验交给 UserService。
class LoginWidget : public QWidget
{
    Q_OBJECT
private:
    QPushButton *btn_login;
    QLineEdit *edit_user;
    QLineEdit *edit_pwd;
    QLineEdit *l_vode;
    VerifyCodeLabel *m_verifyLabel;

public:
    explicit LoginWidget(QWidget *parent = nullptr);
    void init_connect();

signals:
    void login_sucess(QString);

public slots:
    void login();
};

#endif // LOGINWIDGET_H
