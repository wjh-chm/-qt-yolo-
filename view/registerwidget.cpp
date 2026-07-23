#include "registerwidget.h"

registerwidget::registerwidget(QWidget *parent) : QWidget(parent)
{
    setWindowTitle(QStringLiteral("注册界面"));
    resize(800, 600);

    // 背景处理与登录页保持一致，保证界面风格统一。
    QPixmap pm(":/image/R.jpg");
    QPalette pal = palette();
    pal.setBrush(QPalette::Window, QBrush(pm.scaled(size())));
    setAutoFillBackground(true);
    setPalette(pal);

    QVBoxLayout *main_layout = new QVBoxLayout;
    setLayout(main_layout);

    QLabel *lab_title = new QLabel(QStringLiteral("XX系统注册窗口"));
    lab_title->setFont(QFont(QStringLiteral("宋体"), 20, QFont::Bold));
    lab_title->setAlignment(Qt::AlignCenter);

    QHBoxLayout *h_layout = new QHBoxLayout;
    QFormLayout *form_layout = new QFormLayout;
    QLineEdit *edit_user = new QLineEdit;
    QLineEdit *edit_pwd = new QLineEdit;
    QLineEdit *edit_repwd = new QLineEdit;
    form_layout->addRow(QStringLiteral("用户名"), edit_user);
    form_layout->addRow(QStringLiteral("密码"), edit_pwd);
    form_layout->addRow(QStringLiteral("确认密码"), edit_repwd);

    // 当前页面只有界面骨架，还没有接入真正的注册流程。
    QHBoxLayout *btn_layout = new QHBoxLayout;
    QPushButton *btn_reg = new QPushButton(QStringLiteral("注册"));
    QPushButton *btn_cancel = new QPushButton(QStringLiteral("取消"));
    btn_layout->addWidget(btn_reg);
    btn_layout->addWidget(btn_cancel);
    form_layout->addRow(btn_layout);

    form_layout->setSpacing(30);
    h_layout->addStretch();
    h_layout->addLayout(form_layout);
    h_layout->addStretch();

    main_layout->addStretch(2);
    main_layout->addWidget(lab_title);
    main_layout->addStretch(1);
    main_layout->addLayout(h_layout);
    main_layout->addStretch(2);
}
