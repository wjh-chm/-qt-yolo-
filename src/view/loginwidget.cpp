#include "loginwidget.h"

#include "net/clientapi.h"
#include "util/winmannager.h"

#include <QBrush>
#include <QFont>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPalette>
#include <QPixmap>
#include <QVBoxLayout>

LoginWidget::LoginWidget(QWidget *parent) : QWidget(parent)
{
    setWindowTitle(QStringLiteral("登录界面"));
    resize(800, 600);

    QPixmap pm(":/image/back.jpg");
    QPalette pal = palette();
    pal.setBrush(QPalette::Window, QBrush(pm.scaled(size())));
    setAutoFillBackground(true);
    setPalette(pal);

    QVBoxLayout *main_layout = new QVBoxLayout(this);

    QLabel *lab_title = new QLabel(QStringLiteral("智能监控系统登录窗口"));
    lab_title->setFont(QFont(QStringLiteral("宋体"), 20, QFont::Bold));
    lab_title->setAlignment(Qt::AlignCenter);

    QHBoxLayout *h_layout = new QHBoxLayout;
    QFormLayout *form_layout = new QFormLayout;

    edit_user = new QLineEdit(this);
    edit_pwd = new QLineEdit(this);
    edit_pwd->setEchoMode(QLineEdit::Password);

    form_layout->addRow(QStringLiteral("用户名"), edit_user);
    form_layout->addRow(QStringLiteral("密码"), edit_pwd);

    QHBoxLayout *vcode_layout = new QHBoxLayout;
    QLabel *lab_vode = new QLabel(QStringLiteral("验证码"), this);
    l_vode = new QLineEdit(this);
    m_verifyLabel = new VerifyCodeLabel(this);
    m_verifyLabel->setFixedSize(120, 40);
    vcode_layout->addWidget(lab_vode);
    vcode_layout->addWidget(l_vode);
    vcode_layout->addWidget(m_verifyLabel);
    form_layout->addRow(vcode_layout);

    QHBoxLayout *btn_layout = new QHBoxLayout;
    btn_login = new QPushButton(QStringLiteral("登录"), this);
    QPushButton *btn_cancel = new QPushButton(QStringLiteral("取消"), this);
    btn_layout->addWidget(btn_login);
    btn_layout->addWidget(btn_cancel);
    form_layout->addRow(btn_layout);

    form_layout->setSpacing(30);
    h_layout->addStretch();
    h_layout->addLayout(form_layout);
    h_layout->addStretch();

    main_layout->addWidget(lab_title);
    main_layout->addLayout(h_layout);

    connect(btn_cancel, &QPushButton::clicked, this, [this]() {
        hide();
        WinMannager::getInstence()->mainwidget.show();
    });

    init_connect();
}

void LoginWidget::init_connect()
{
    connect(btn_login, &QPushButton::clicked, this, &LoginWidget::login);
}

void LoginWidget::setClientApi(ClientApi *api)
{
    m_clientApi = api;
    if (m_clientApi == nullptr) {
        return;
    }

    connect(m_clientApi, &ClientApi::loginFinished,
            this, &LoginWidget::onLoginFinished,
            Qt::UniqueConnection);
    connect(m_clientApi, &ClientApi::errorOccurred,
            this, &LoginWidget::onNetworkError,
            Qt::UniqueConnection);
}

void LoginWidget::login()
{
    const QString username = edit_user->text().trimmed();
    const QString userpwd = edit_pwd->text().trimmed();

    if (username.isEmpty() || userpwd.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("登录提示"), QStringLiteral("请输入用户名和密码"));
        return;
    }

    const QString inputcode = l_vode->text().trimmed();
    const QString realCode = m_verifyLabel->getCode();
    if (inputcode.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入验证码"));
        l_vode->setFocus();
        return;
    }
    if (inputcode.toUpper() != realCode) {
        QMessageBox::critical(this, QStringLiteral("提示"), QStringLiteral("验证码输入不正确"));
        l_vode->clear();
        m_verifyLabel->generateCode();
        l_vode->setFocus();
        return;
    }

    if (m_clientApi == nullptr || !m_clientApi->isConnected()) {
        QMessageBox::warning(this, QStringLiteral("登录警告"), QStringLiteral("服务端未连接，请确认 Linux 服务端已启动"));
        return;
    }

    m_pendingUsername = username;
    m_loginRequesting = true;
    btn_login->setEnabled(false);
    m_loginRequestId = m_clientApi->login(username, userpwd);
}

void LoginWidget::onLoginFinished(qint64 requestId,bool success, const QString &message)
{
    if (!m_loginRequesting || requestId != m_loginRequestId) {
        return;
    }

    m_loginRequesting = false;
     m_loginRequestId = 0;
    btn_login->setEnabled(true);

    if (success) {
        WinMannager::getInstence()->mainwidget.showusrname(m_pendingUsername);
        return;
    }

    QMessageBox::warning(this, QStringLiteral("登录警告"),
                         message.isEmpty() ? QStringLiteral("用户名或密码错误") : message);
    edit_pwd->clear();
    edit_user->clear();
    l_vode->clear();
    m_verifyLabel->generateCode();
}

void LoginWidget::onNetworkError(const QString &message)
{
    if (!m_loginRequesting) {
        return;
    }

    m_loginRequesting = false;//“有没有正在等登录
    m_loginRequestId = 0;//“正在等哪一次登录”
    btn_login->setEnabled(true);
    QMessageBox::warning(this, QStringLiteral("网络错误"), message);
}
