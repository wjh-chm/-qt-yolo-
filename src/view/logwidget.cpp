#include "logwidget.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QVBoxLayout>

LogWidget::LogWidget(QWidget *parent)
    : QWidget(parent),
      m_table(nullptr),
      m_btn_operation(nullptr),
      m_btn_exception(nullptr),
      m_btn_prev(nullptr),
      m_btn_next(nullptr),
      m_lab_page(nullptr),
      m_mode(LogModel::Mode::Operation),
      m_pageSize(8),
      m_curPage(1),
      m_totalPage(1)
{
    initUi();
    refreshLogs();
}

void LogWidget::refreshLogs()
{
    loadPage(1);
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
    m_table->setHorizontalHeaderLabels({QStringLiteral("日志ID"),
                                        QStringLiteral("日志操作"),
                                        QStringLiteral("操作员"),
                                        QStringLiteral("操作时间")});
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
        m_mode = LogModel::Mode::Operation;
        loadPage(1);
    });
    connect(m_btn_exception, &QPushButton::clicked, this, [this]() {
        m_mode = LogModel::Mode::Exception;
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
    const int count = m_logModel.count(m_mode);
    m_totalPage = qMax(1, (count + m_pageSize - 1) / m_pageSize);
    m_curPage = qBound(1, page, m_totalPage);

    m_table->clearContents();
    m_table->setRowCount(m_pageSize);

    const int offset = (m_curPage - 1) * m_pageSize;
    const QList<LogModel::Record> records = m_logModel.queryPage(m_mode, offset, m_pageSize);

    int row = 0;
    for (const LogModel::Record &record : records) {
        if (row >= m_pageSize) {
            break;
        }

        m_table->setItem(row, 0, new QTableWidgetItem(QString::number(record.id)));
        m_table->setItem(row, 1, new QTableWidgetItem(record.operation));
        m_table->setItem(row, 2, new QTableWidgetItem(record.operatorId));
        m_table->setItem(row, 3, new QTableWidgetItem(record.operateTime));
        ++row;
    }

    m_lab_page->setText(QString("%1 / %2").arg(m_curPage).arg(m_totalPage));
    m_btn_prev->setEnabled(m_curPage > 1);
    m_btn_next->setEnabled(m_curPage < m_totalPage);
    updateModeButtonState();
}

void LogWidget::updateModeButtonState()
{
    m_btn_operation->setEnabled(m_mode != LogModel::Mode::Operation);
    m_btn_exception->setEnabled(m_mode != LogModel::Mode::Exception);
}
