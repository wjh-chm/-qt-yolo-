#include "loginwidget.h"

#include "util/winmannager.h"

LoginWidget::LoginWidget(QWidget *parent) : QWidget(parent)
{
    setWindowTitle(QStringLiteral("登录界面"));
    resize(800, 600);

    // 登录页使用缩放背景图，让演示界面更完整。
    QPixmap pm(":/image/back.jpg");
    QPalette pal = palette();
    pal.setBrush(QPalette::Window, QBrush(pm.scaled(size())));
    setAutoFillBackground(true);
    setPalette(pal);

    QVBoxLayout *main_layout = new QVBoxLayout;
    setLayout(main_layout);

    QLabel *lab_title = new QLabel(QStringLiteral("XX系统登录窗口"));
    lab_title->setFont(QFont(QStringLiteral("宋体"), 20, QFont::Bold));
    lab_title->setAlignment(Qt::AlignCenter);

    QHBoxLayout *h_layout = new QHBoxLayout;
    QFormLayout *form_layout = new QFormLayout;
    edit_user = new QLineEdit;
    edit_pwd = new QLineEdit;
    edit_pwd->setEchoMode(QLineEdit::Password);
    form_layout->addRow(QStringLiteral("用户名"), edit_user);
    form_layout->addRow(QStringLiteral("密码"), edit_pwd);

    // 验证码在本地生成，输错后会重新刷新。
    QHBoxLayout *vcode_layout = new QHBoxLayout;
    QLabel *lab_vode = new QLabel(QStringLiteral("验证码"));
    l_vode = new QLineEdit;
    m_verifyLabel = new VerifyCodeLabel;
    m_verifyLabel->setFixedSize(120, 40);
    vcode_layout->addWidget(lab_vode);
    vcode_layout->addWidget(l_vode);
    vcode_layout->addWidget(m_verifyLabel);
    form_layout->addRow(vcode_layout);

    QHBoxLayout *btn_layout = new QHBoxLayout;
    btn_login = new QPushButton(QStringLiteral("登录"));
    //QPushButton *btn_reg = new QPushButton(QStringLiteral("注册"));
    QPushButton *btn_cancel = new QPushButton(QStringLiteral("取消"));
    btn_layout->addWidget(btn_login);
    //btn_layout->addWidget(btn_reg);
    btn_layout->addWidget(btn_cancel);
    form_layout->addRow(btn_layout);

    form_layout->setSpacing(30);
    h_layout->addStretch();
    h_layout->addLayout(form_layout);
    h_layout->addStretch();

    main_layout->addWidget(lab_title);
    main_layout->addLayout(h_layout);

    init_connect();
}

void LoginWidget::init_connect()
{
    connect(btn_login, SIGNAL(clicked()), this, SLOT(login()));
}

void LoginWidget::login()
{
    QString username = edit_user->text().trimmed();
    QString userpwd = edit_pwd->text().trimmed();

    // 先校验验证码，再访问数据库做账号密码校验。
    QString inputcode = l_vode->text().trimmed();
    QString realCode = m_verifyLabel->getCode();
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

    UserService userService;
    bool result = userService.login(username, userpwd);
    if (result) {
        WinMannager::getInstence()->mainwidget.showusrname(username);
    } else {
        QMessageBox::warning(this, QStringLiteral("登录警告"), QStringLiteral("用户名或密码错误"));
        edit_pwd->clear();
        edit_user->clear();
        l_vode->clear();
        m_verifyLabel->generateCode();
    }
}
