#ifndef LOGINWIDGET_H
#define LOGINWIDGET_H

#include "verifycodelabel.h"

#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QWidget>

class ClientApi;

class LoginWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LoginWidget(QWidget *parent = nullptr);

    void init_connect();
    void setClientApi(ClientApi *api);

signals:
    void login_sucess(QString);

public slots:
    void login();
    void onLoginFinished(qint64 requestId,
                         bool success,
                         const QString &message);
    void onNetworkError(const QString &message);

private:
    QPushButton *btn_login = nullptr;
    QLineEdit *edit_user = nullptr;
    QLineEdit *edit_pwd = nullptr;
    QLineEdit *l_vode = nullptr;
    VerifyCodeLabel *m_verifyLabel = nullptr;
    ClientApi *m_clientApi = nullptr;
    QString m_pendingUsername;
    bool m_loginRequesting = false;
    qint64 m_loginRequestId = 0;

};

#endif // LOGINWIDGET_H
