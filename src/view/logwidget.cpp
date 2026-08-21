#include "logwidget.h"
#include "net/clientapi.h"

#include <QJsonObject>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QDebug>


LogWidget::LogWidget(QWidget *parent)
    : QWidget(parent),
      m_table(nullptr),
      m_btn_operation(nullptr),
      m_btn_exception(nullptr),
      m_btn_prev(nullptr),
      m_btn_next(nullptr),
      m_lab_page(nullptr),
      m_mode(Mode::Operation),
      m_pageSize(8),
      m_curPage(1),
      m_totalPage(1)
{
    initUi();
}

void LogWidget::refreshLogs()
{
    loadPage(1);
}

void LogWidget::setClientApi(ClientApi *api)
{
    if (m_clientApi == api) {
        return;
    }

    if (m_clientApi != nullptr) {
        disconnect(m_clientApi, nullptr, this, nullptr);
    }

    m_clientApi = api;
    if (m_clientApi == nullptr) {
        return;
    }

    connect(m_clientApi,
            &ClientApi::logListFinished,
            this,
            &LogWidget::onLogListFinished,
            Qt::UniqueConnection);
}

void LogWidget::initUi()
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(36, 36, 36, 30);
    mainLayout->setSpacing(48);

    QWidget *leftPanel = new QWidget(this);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 90, 0, 0);
    leftLayout->setSpacing(42);

    m_btn_operation = new QPushButton(QStringLiteral("操作日志"), this);
    m_btn_exception = new QPushButton(QStringLiteral("异常日志"), this);
    m_btn_operation->setMinimumSize(118, 64);
    m_btn_exception->setMinimumSize(118, 64);
    leftLayout->addWidget(m_btn_operation);
    leftLayout->addWidget(m_btn_exception);
    leftLayout->addStretch();
    mainLayout->addWidget(leftPanel, 0);

    QWidget *rightPanel = new QWidget(this);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 70, 0, 0);
    rightLayout->setSpacing(24);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    updateTableHeader();
    m_table->setRowCount(m_pageSize);
    m_table->verticalHeader()->setVisible(false);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->verticalHeader()->setDefaultSectionSize(58);
    rightLayout->addWidget(m_table, 1);

    QHBoxLayout *pageLayout = new QHBoxLayout;
    pageLayout->addStretch();
    m_btn_prev = new QPushButton(QStringLiteral("<"), this);
    m_lab_page = new QLabel(QStringLiteral("1 / 1"), this);
    m_btn_next = new QPushButton(QStringLiteral(">"), this);
    m_btn_prev->setFixedSize(34, 30);
    m_btn_next->setFixedSize(34, 30);
    m_lab_page->setAlignment(Qt::AlignCenter);
    m_lab_page->setMinimumWidth(72);
    pageLayout->addWidget(m_btn_prev);
    pageLayout->addWidget(m_lab_page);
    pageLayout->addWidget(m_btn_next);
    pageLayout->addStretch();
    rightLayout->addLayout(pageLayout);

    mainLayout->addWidget(rightPanel, 1);

    connect(m_btn_operation, &QPushButton::clicked, this, [this]() {
        m_mode = Mode::Operation;
        updateTableHeader();
        loadPage(1);
    });
    connect(m_btn_exception, &QPushButton::clicked, this, [this]() {
        m_mode = Mode::Exception;
        updateTableHeader();
        loadPage(1);
    });
    connect(m_btn_prev, &QPushButton::clicked, this, [this]() {
        if (m_curPage > 1) {
            loadPage(m_curPage - 1);
        }
    });
    connect(m_btn_next, &QPushButton::clicked, this, [this]() {
        if (m_curPage < m_totalPage) {
            loadPage(m_curPage + 1);
        }
    });

    updateModeButtonState();
}

void LogWidget::loadPage(int page)
{
    if (m_clientApi == nullptr || !m_clientApi->isConnected()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("服务端未连接"));
        return;
    }

    m_curPage = qMax(1, page);

    m_table->clearContents();
    m_table->setRowCount(m_pageSize);

    m_lab_page->setText(QStringLiteral("加载中..."));
    m_btn_prev->setEnabled(false);
    m_btn_next->setEnabled(false);

    QString modeText;
    if (m_mode == Mode::Exception) {
        modeText = QStringLiteral("exception");
    } else {
        modeText = QStringLiteral("operation");
    }

    m_waitingLogList = true;
    m_logListRequestId = m_clientApi->queryLogList(modeText, m_curPage, m_pageSize);//发送请求

    updateModeButtonState();
}

void LogWidget::updateModeButtonState()
{
    m_btn_operation->setEnabled(m_mode != Mode::Operation);
    m_btn_exception->setEnabled(m_mode != Mode::Exception);
}

void LogWidget::updateTableHeader()
{
    if (m_mode == Mode::Exception) {
        m_table->setHorizontalHeaderLabels({
            QStringLiteral("日志ID"),
            QStringLiteral("通道"),
            QStringLiteral("相关视频路径"),
            QStringLiteral("发生时间")
        });
    } else {
        m_table->setHorizontalHeaderLabels({
            QStringLiteral("日志ID"),
            QStringLiteral("日志操作"),
            QStringLiteral("操作员"),
            QStringLiteral("操作时间")
        });
    }
}

void LogWidget::onLogListFinished(qint64 requestId,
                                  bool success,
                                  const QJsonArray &list,
                                  int totalCount,
                                  const QString &message)
{
    if (!m_waitingLogList || requestId != m_logListRequestId) {
        return;
    }

    m_waitingLogList = false;
    m_logListRequestId = 0;

    if (!success) {
        QMessageBox::warning(this,
                             QStringLiteral("查询失败"),
                             message.isEmpty() ? QStringLiteral("日志查询失败") : message);

        m_lab_page->setText(QString("%1 / ?").arg(m_curPage));
        m_btn_prev->setEnabled(m_curPage > 1);
        m_btn_next->setEnabled(false);
        return;
    }
    m_table->clearContents();
    m_table->setRowCount(m_pageSize);
    int row = 0;

    for (const QJsonValue &value : list) {
        if (row >= m_pageSize) {
            break;
        }

        QJsonObject item = value.toObject();

        if (m_mode == Mode::Exception) {
            m_table->setItem(row, 0, new QTableWidgetItem(QString::number(item.value("id").toInt())));
            m_table->setItem(row, 1, new QTableWidgetItem(QStringLiteral("通道%1").arg(item.value("channel_id").toInt())));
            m_table->setItem(row, 2, new QTableWidgetItem(item.value("related_videopath").toString()));
            m_table->setItem(row, 3, new QTableWidgetItem(item.value("event_time").toString()));
        }
        else {
            m_table->setItem(row, 0, new QTableWidgetItem(QString::number(item.value("id").toInt())));
            m_table->setItem(row, 1, new QTableWidgetItem(item.value("operation_desc").toString()));
            m_table->setItem(row, 2, new QTableWidgetItem(QString::number(item.value("admin_id").toInt())));
            m_table->setItem(row, 3, new QTableWidgetItem(item.value("operation_time").toString()));
        }
        ++row;
    }
    m_totalPage = qMax(1, (totalCount + m_pageSize - 1) / m_pageSize);

    m_lab_page->setText(QString("%1 / %2").arg(m_curPage).arg(m_totalPage));
    m_btn_prev->setEnabled(m_curPage > 1);
    m_btn_next->setEnabled(m_curPage < m_totalPage);

    updateModeButtonState();

}
