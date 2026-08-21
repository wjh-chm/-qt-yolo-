#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include "exceptionwidget.h"
#include "logwidget.h"
#include "monitorwidget.h"
#include "reviewwidget.h"
#include "settingwidget.h"

#include <QDateTime>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedLayout>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

class ClientApi;

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MainWidget(QWidget *parent = nullptr);
    void init_top_widget();
    void showusrname(const QString usrname);
    void init_qss();
    void init_connect();
    void init_showtime();
    bool isLoggedIn() const;
    void setClientApi(ClientApi *api);

public slots:
    void login();
    void back_main_win();
    void update_time();
    void switchToMonitor();
    void switchToReview();
    void switchToAnomalyReview();
    void switchToLogView();
    void switchToSetting();

private:
    QHBoxLayout *top_layout;
    QStackedLayout *bottom_layout;

    QPushButton *btn_login;
    QPushButton *btn_jiankong;
    QPushButton *btn_review;
    QPushButton *btn_anomaly_review;
    QPushButton *btn_log_view;
    QPushButton *btn_setting;

    QLabel *lab_login;
    QLabel *lab_time;

    Monitorwidget *m_widget;
    ReviewWidget *m_review_widget;
    ExceptionWidget *m_exception_widget;
    LogWidget *m_log_widget;
    SettingWidget *m_setting_widget;
    QString m_loginUser;
};

#endif // MAINWIDGET_H
