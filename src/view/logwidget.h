#ifndef LOGWIDGET_H
#define LOGWIDGET_H

#include <QJsonArray>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QWidget>

class ClientApi;

class LogWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LogWidget(QWidget *parent = nullptr);
    void refreshLogs();
    void setClientApi(ClientApi *api);

private:
    enum class Mode {
        Operation,
        Exception
    };

    void initUi();
    void loadPage(int page);
    void updateModeButtonState();
    void updateTableHeader();

    QTableWidget *m_table;
    QPushButton *m_btn_operation;
    QPushButton *m_btn_exception;
    QPushButton *m_btn_prev;
    QPushButton *m_btn_next;
    QLabel *m_lab_page;

    Mode m_mode;
    int m_pageSize;
    int m_curPage;
    int m_totalPage;
    bool m_waitingLogList = false;
    qint64 m_logListRequestId = 0;
    ClientApi *m_clientApi = nullptr;

private slots:
    void onLogListFinished(qint64 requestId,
                           bool success,
                           const QJsonArray &list,
                           int totalCount,
                           const QString &message);
};

#endif // LOGWIDGET_H
