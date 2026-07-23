#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include "monitorwidget.h"
#include "reviewwidget.h"
#include "settingwidget.h"
#include <QDateTime>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QPalette>
#include <QPushButton>
#include <QStackedLayout>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

// 主容器窗口：承载顶部导航栏，以及监控页和回放页两个主页面。
class MainWidget : public QWidget
{
    Q_OBJECT
private:
    QHBoxLayout *top_layout;      // 顶部导航区域布局，放登录、监控、回放和时间。
    QStackedLayout *bottom_layout; // 下半区页面栈，0 为监控页，1 为回放页。
    QPushButton *btn_login;       // 右上角登录按钮，点击后进入登录页。
    QPushButton *btn_jiankong;    // 顶部“监控”按钮，用于切回监控页。
    QPushButton *btn_review;      // 顶部“回放”按钮，进入回放前会检查登录状态。
    QPushButton *btn_setting;// 顶部“设置”按钮
    QLabel *lab_login;            // 显示当前登录用户名，未登录时显示游客状态。
    QLabel *lab_time;             // 顶部时钟标签，每秒刷新一次当前时间。

    Monitorwidget *m_widget;          // 监控页面对象，负责实时预览和录像控制。
    ReviewWidget *m_review_widget;    // 回放页面对象，负责历史录像查看。
    SettingWidget *m_setting_widget;  //设置对象，负责主要路径设置
    QString m_loginUser;              // 当前登录用户名称，为空表示未登录。

public:
    explicit MainWidget(QWidget *parent = nullptr);
    void init_top_widget();
    void showusrname(const QString usrname);
    void init_qss();
    void init_connect();
    void init_showtime();
    bool isLoggedIn() const;

signals:

public slots:
    void login();
    void back_main_win();
    void update_time();
    void switchToMonitor();
    void switchToReview();
    void switchToSetting();
};

#endif // MAINWIDGET_H
