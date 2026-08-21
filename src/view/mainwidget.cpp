#include "mainwidget.h"

#include "net/clientapi.h"
#include "util/winmannager.h"

#include <QIcon>
#include <QMessageBox>

MainWidget::MainWidget(QWidget *parent) : QWidget(parent)
{
    init_qss();
    setWindowTitle(QStringLiteral("智能监控系统主页"));
    setMinimumSize(1600, 1000);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    top_layout = new QHBoxLayout;
    bottom_layout = new QStackedLayout;

    m_widget = new Monitorwidget;
    m_review_widget = new ReviewWidget(ReviewWidget::ReviewScope::NormalOnly);
    m_exception_widget = new ExceptionWidget;
    m_log_widget = new LogWidget;
    m_setting_widget = new SettingWidget;

    bottom_layout->addWidget(m_widget);
    bottom_layout->addWidget(m_review_widget);
    bottom_layout->addWidget(m_exception_widget);
    bottom_layout->addWidget(m_log_widget);
    bottom_layout->addWidget(m_setting_widget);
    bottom_layout->setCurrentIndex(0);

    mainLayout->addLayout(top_layout, 1);
    mainLayout->addLayout(bottom_layout, 8);

    init_top_widget();
    init_connect();
    init_showtime();
}

void MainWidget::init_top_widget()
{
    QLabel *labTitle = new QLabel(QStringLiteral("安防监控管理平台"));
    labTitle->setObjectName("title");
    top_layout->addWidget(labTitle);
    top_layout->addStretch();

    btn_jiankong = new QPushButton(QIcon(":/image/jiankong.png"), QStringLiteral("监控"));
    btn_review = new QPushButton(QIcon(":/image/shipin.png"), QStringLiteral("普通回放"));
    btn_anomaly_review = new QPushButton(QIcon(":/image/shipin.png"), QStringLiteral("移动侦测回看"));
    btn_log_view = new QPushButton(QIcon(":/image/shezhi.png"), QStringLiteral("日志查看"));
    btn_setting = new QPushButton(QIcon(":/image/shezhi.png"), QStringLiteral("设置"));

    top_layout->addWidget(btn_jiankong);
    top_layout->addWidget(btn_review);
    top_layout->addWidget(btn_anomaly_review);
    top_layout->addWidget(btn_log_view);
    top_layout->addWidget(btn_setting);

    lab_time = new QLabel(this);
    lab_time->setObjectName("ctime");
    top_layout->addWidget(lab_time);

    QWidget *loginBox = new QWidget(this);
    QVBoxLayout *loginLayout = new QVBoxLayout(loginBox);
    btn_login = new QPushButton(this);
    btn_login->setIcon(QIcon(":/image/login.png"));
    btn_login->setIconSize(QSize(40, 50));
    lab_login = new QLabel(QStringLiteral("未登录"), this);
    loginLayout->addWidget(btn_login);
    loginLayout->addWidget(lab_login);
    top_layout->addWidget(loginBox);
}

void MainWidget::showusrname(const QString usrname)
{
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

void MainWidget::setClientApi(ClientApi *api)
{
    if (m_widget != nullptr) {
        m_widget->setClientApi(api);
    }
    if (m_review_widget != nullptr) {
        m_review_widget->setClientApi(api);
    }
    if (m_exception_widget != nullptr) {
        m_exception_widget->setClientApi(api);
    }
    if (m_log_widget != nullptr) {
        m_log_widget->setClientApi(api);
    }
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
    lab_time->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
}

void MainWidget::switchToMonitor()
{
    bottom_layout->setCurrentIndex(0);
}

void MainWidget::switchToReview()
{
    if (!isLoggedIn()) {
        QMessageBox::information(this, QStringLiteral("访问受限"), QStringLiteral("请先登录后再查看普通回放。"));
        bottom_layout->setCurrentIndex(0);
        return;
    }

    m_review_widget->refreshData();
    bottom_layout->setCurrentIndex(1);
}

void MainWidget::switchToAnomalyReview()
{
    if (!isLoggedIn()) {
        QMessageBox::information(this, QStringLiteral("访问受限"), QStringLiteral("请先登录后再查看移动侦测回看。"));
        bottom_layout->setCurrentIndex(0);
        return;
    }

    m_exception_widget->refreshData();
    bottom_layout->setCurrentIndex(2);
}

void MainWidget::switchToLogView()
{
    if (!isLoggedIn()) {
        QMessageBox::information(this, QStringLiteral("访问受限"), QStringLiteral("请先登录后再查看异常日志。"));
        bottom_layout->setCurrentIndex(0);
        return;
    }

    m_log_widget->refreshLogs();
    bottom_layout->setCurrentIndex(3);
}

void MainWidget::switchToSetting()
{
    if (!isLoggedIn()) {
        QMessageBox::information(this, QStringLiteral("访问受限"), QStringLiteral("请先登录后再进入设置页面。"));
        bottom_layout->setCurrentIndex(0);
        return;
    }

    bottom_layout->setCurrentIndex(4);
}

void MainWidget::init_connect()
{
    connect(btn_login, &QPushButton::clicked, this, &MainWidget::login);
    connect(btn_jiankong, &QPushButton::clicked, this, &MainWidget::switchToMonitor);
    connect(btn_review, &QPushButton::clicked, this, &MainWidget::switchToReview);
    connect(btn_anomaly_review, &QPushButton::clicked, this, &MainWidget::switchToAnomalyReview);
    connect(btn_log_view, &QPushButton::clicked, this, &MainWidget::switchToLogView);
    connect(btn_setting, &QPushButton::clicked, this, &MainWidget::switchToSetting);
    connect(m_setting_widget, &SettingWidget::settingsSaved, m_widget, &Monitorwidget::reloadStorageSettings);
    connect(m_setting_widget, &SettingWidget::requestBackToMain, this, [this]() {
        switchToMonitor();
    });
}

void MainWidget::init_showtime()
{
    QTimer *timer = new QTimer(this);
    timer->setInterval(1000);
    connect(timer, &QTimer::timeout, this, &MainWidget::update_time);
    timer->start();
    update_time();
}
