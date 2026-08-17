#ifndef LOGWIDGET_H
#define LOGWIDGET_H

#include "model/logmodel.h"

#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QWidget>

class LogWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LogWidget(QWidget *parent = nullptr);
    void refreshLogs();

private:
    void initUi();
    void loadPage(int page);
    void updateModeButtonState();

    QTableWidget *m_table;
    QPushButton *m_btn_operation;
    QPushButton *m_btn_exception;
    QPushButton *m_btn_prev;
    QPushButton *m_btn_next;
    QLabel *m_lab_page;

    LogModel m_logModel;
    LogModel::Mode m_mode;
    int m_pageSize;
    int m_curPage;
    int m_totalPage;
};

#endif // LOGWIDGET_H
