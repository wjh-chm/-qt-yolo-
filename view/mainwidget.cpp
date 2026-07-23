#include "mainwidget.h"

#include <QMessageBox>

#include "../util/winmannager.h"

MainWidget::MainWidget(QWidget *parent) : QWidget(parent)
{
    init_qss();
    setWindowTitle(QStringLiteral("智能监控系统主页"));
    setMinimumSize(1600, 1000);

    QVBoxLayout *main_layout = new QVBoxLayout;
    setLayout(main_layout);

    top_layout = new QHBoxLayout;
    bottom_layout = new QStackedLayout;

    m_widget = new Monitorwidget;
    bottom_layout->addWidget(m_widget);

    m_review_widget = new ReviewWidget;
    bottom_layout->addWidget(m_review_widget);

    m_setting_widget=new SettingWidget;
    bottom_layout->addWidget(m_setting_widget);
    // 无论游客还是已登录用户，默认都先进入监控页。
    bottom_layout->setCurrentIndex(0);

    main_layout->addLayout(top_layout, 1);
    main_layout->addLayout(bottom_layout, 8);

    init_top_widget();
    init_connect();
    init_showtime();
}

void MainWidget::init_top_widget()
{
    QLabel *lab_title = new QLabel(QStringLiteral("安防监控管理平台"));
    lab_title->setObjectName("title");
    top_layout->addWidget(lab_title);
    top_layout->addStretch();

    btn_jiankong = new QPushButton(QIcon(":/image/jiankong.png"), QStringLiteral("监控"));
    top_layout->addWidget(btn_jiankong);
    btn_review = new QPushButton(QIcon(":/image/shipin.png"), QStringLiteral("回放"));
    top_layout->addWidget(btn_review);
    btn_setting = new QPushButton(QIcon(":/image/shezhi.png"), QStringLiteral("设置"));
    top_layout->addWidget(btn_setting);

    lab_time = new QLabel("");
    lab_time->setObjectName("ctime");
    top_layout->addWidget(lab_time);

    QWidget *btn_login_box = new QWidget;
    QVBoxLayout *btn_login_layout = new QVBoxLayout;
    btn_login_box->setLayout(btn_login_layout);
    btn_login = new QPushButton("");
    btn_login->setIcon(QIcon(":/image/login.png"));
    btn_login->setIconSize(QSize(40, 50));
    btn_login_layout->addWidget(btn_login);
    lab_login = new QLabel(QStringLiteral("未登录"));
    btn_login_layout->addWidget(lab_login);

    top_layout->addWidget(btn_login_box);
}

void MainWidget::showusrname(const QString usrname)
{
    // 登录成功后，同时更新主界面显示和监控页的权限状态。
    m_loginUser = usrname;
    WinMannager::getInstence()->mainwidget.show();
    WinMannager::getInstence()->loginwidget.hide();
    lab_login->setText(usrname);
    if (m_widget != nullptr) {
        m_widget->setLoginUser(usrname);
    }
}

void MainWidget::init_qss()
{
    QFile file(":/qss/main.qss");
    if (file.open(QFile::ReadOnly)) {
        setStyleSheet(file.readAll());
    }
}

bool MainWidget::isLoggedIn() const
{
    return !m_loginUser.isEmpty();
}

void MainWidget::login()
{
    WinMannager::getInstence()->loginwidget.show();
    hide();
}

void MainWidget::back_main_win()
{
    WinMannager::getInstence()->loginwidget.hide();
    show();
}

void MainWidget::update_time()
{
    const QDateTime currenTime = QDateTime::currentDateTime();
    const QString timeString = currenTime.toString("yyyy-MM-dd  HH:mm:ss");
    lab_time->setText(timeString);
}

void MainWidget::switchToMonitor()
{
    bottom_layout->setCurrentIndex(0);
}

void MainWidget::switchToReview()
{
    //游客允许看实时预览，但回放页必须登录后才能进入。
    if (!isLoggedIn()) {
        QMessageBox::information(this, QStringLiteral("访问受限"), QStringLiteral("请先登录，登录后才可查看回放记录。"));
        bottom_layout->setCurrentIndex(0);
        return;
    }
    bottom_layout->setCurrentIndex(1);
}

void MainWidget::switchToSetting()
{
    if (!isLoggedIn()) {
        QMessageBox::information(this, QStringLiteral("访问受限"), QStringLiteral("请先登录，登录后才可设置。"));
        bottom_layout->setCurrentIndex(0);
        return;
    }
    bottom_layout->setCurrentIndex(2);
}

void MainWidget::init_connect()
{
    connect(btn_login, SIGNAL(clicked()), this, SLOT(login()));
    connect(btn_jiankong, SIGNAL(clicked()), this, SLOT(switchToMonitor()));
    connect(btn_review, SIGNAL(clicked()), this, SLOT(switchToReview()));
    connect(btn_setting, SIGNAL(clicked()), this, SLOT(switchToSetting()));
    connect(m_setting_widget, &SettingWidget::settingsSaved, m_widget, &Monitorwidget::reloadStorageSettings);
    connect(m_setting_widget, &SettingWidget::requestBackToMain, this, [this]() {
        switchToMonitor();
    });
}

void MainWidget::init_showtime()
{
    // 顶部时钟由界面持有的定时器每秒刷新一次。
    QTimer *m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(update_time()));
    m_timer->start();
    update_time();
}
