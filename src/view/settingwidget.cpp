#include "settingwidget.h"

#include "util/appsettings.h"

#include <QCameraDevice>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QMediaDevices>
#include <QMessageBox>
#include <QVBoxLayout>

SettingWidget::SettingWidget(QWidget *parent)
    : QWidget(parent)
{
    initUi();
    initQss();

    connect(btn_cancel, &QPushButton::clicked, this, [this]() {
        loadSetting();
        emit requestBackToMain();
    });
    connect(btn_browse, &QPushButton::clicked, this, &SettingWidget::browseRecordRoot);
    connect(btn_save, &QPushButton::clicked, this, &SettingWidget::saveSetting);

    loadSetting();
}

void SettingWidget::initUi()
{
    setObjectName("setting_page");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(40, 30, 40, 30);
    mainLayout->setSpacing(20);
    mainLayout->addStretch();

    lab_title = new QLabel(QStringLiteral("智能监控系统设置页"), this);
    lab_title->setAlignment(Qt::AlignCenter);
    QFont font = lab_title->font();
    font.setPointSize(22);
    font.setBold(true);
    lab_title->setFont(font);
    mainLayout->addWidget(lab_title, 0, Qt::AlignHCenter);

    QWidget *formWidget = new QWidget(this);
    formWidget->setFixedWidth(900);
    QGridLayout *grid = new QGridLayout(formWidget);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(24);
    grid->setVerticalSpacing(22);

    lab_savepath = new QLabel(QStringLiteral("保存路径:"), formWidget);
    edit_path = new QLineEdit(formWidget);
    btn_browse = new QPushButton(QStringLiteral("浏览"), formWidget);
    grid->addWidget(lab_savepath, 0, 0);
    grid->addWidget(edit_path, 0, 1);
    grid->addWidget(btn_browse, 0, 2);

    QLabel *intervalLabel = new QLabel(QStringLiteral("间隔时间:"), formWidget);
    box_interval = new QComboBox(formWidget);
    box_interval->addItem(QStringLiteral("10秒"), 10);
    box_interval->addItem(QStringLiteral("30秒"), 30);
    box_interval->addItem(QStringLiteral("60秒"), 60);
    box_interval->addItem(QStringLiteral("120秒"), 120);
    grid->addWidget(intervalLabel, 1, 0);
    grid->addWidget(box_interval, 1, 1);

    for (int i = 0; i < 4; ++i) {
        m_cameraLabels[i] = new QLabel(QStringLiteral("视频源%1").arg(i + 1), formWidget);
        m_channelNameEdits[i] = new QLineEdit(formWidget);
        m_channelNameEdits[i]->setPlaceholderText(QStringLiteral("通道名称"));

        m_channelSelectCombos[i] = new QComboBox(formWidget);
        m_channelSelectCombos[i]->addItems({
            QStringLiteral("通道1"),
            QStringLiteral("通道2"),
            QStringLiteral("通道3"),
            QStringLiteral("通道4"),
        });
        m_channelSelectCombos[i]->setCurrentIndex(i);

        grid->addWidget(m_cameraLabels[i], i + 2, 0);
        grid->addWidget(m_channelNameEdits[i], i + 2, 1);
        grid->addWidget(m_channelSelectCombos[i], i + 2, 2);
    }

    m_channelNameEdits[0]->setReadOnly(true);

    grid->setColumnStretch(0, 2);
    grid->setColumnStretch(1, 5);
    grid->setColumnStretch(2, 2);

    mainLayout->addWidget(formWidget, 0, Qt::AlignHCenter);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();

    btn_save = new QPushButton(QStringLiteral("保存"), this);
    btn_cancel = new QPushButton(QStringLiteral("取消"), this);

    buttonLayout->addWidget(btn_save);
    buttonLayout->addSpacing(40);
    buttonLayout->addWidget(btn_cancel);
    buttonLayout->addStretch();

    mainLayout->addLayout(buttonLayout);
    mainLayout->addStretch();
}

void SettingWidget::initQss()
{
    QFile file(":/qss/monitor.qss");
    if (file.open(QFile::ReadOnly)) {
        setStyleSheet(file.readAll());
    }
}

void SettingWidget::browseRecordRoot()
{
    QString currentPath = edit_path->text().trimmed();
    if (currentPath.isEmpty() || !QDir(currentPath).exists()) {
        currentPath = AppSettings::defaultRecordRoot();
    }

    const QString dir = QFileDialog::getExistingDirectory(
        this,
        QStringLiteral("选择保存路径"),
        currentPath);
    if (dir.isEmpty()) {
        return;
    }

    edit_path->setText(dir);
}

void SettingWidget::loadSetting()
{
    const StorageSettings settings = AppSettings::loadStorageSettings();
    const QStringList cameraNames = detectedCameraNames();

    refreshCameraLabels();
    edit_path->setText(settings.recordRoot);

    int intervalIndex = box_interval->findData(settings.intervalSeconds);
    if (intervalIndex < 0) {
        intervalIndex = box_interval->findData(AppSettings::defaultIntervalSeconds());
    }
    if (intervalIndex < 0 && box_interval->count() > 0) {
        intervalIndex = 0;
    }
    if (intervalIndex >= 0) {
        box_interval->setCurrentIndex(intervalIndex);
    }

    for (int i = 0; i < 4; ++i) {
        m_channelSelectCombos[i]->setCurrentIndex(settings.channelIndexes[i]);

        if (i == 0) {
            const QString detectedName = cameraNames.isEmpty() ? QStringLiteral("未检测到摄像头")
                                                               : cameraNames.first();
            m_channelNameEdits[i]->setText(detectedName);
        } else {
            m_channelNameEdits[i]->setText(settings.channelNames[i]);
        }
    }
}

void SettingWidget::saveSetting()
{
    StorageSettings settings;
    settings.recordRoot = edit_path->text().trimmed();
    settings.intervalSeconds = box_interval->currentData().toInt();

    for (int i = 0; i < 4; ++i) {
        settings.channelNames[i] = m_channelNameEdits[i]->text().trimmed();
        if (settings.channelNames[i].isEmpty()) {
            settings.channelNames[i] = AppSettings::defaultChannelName(i);
        }
        settings.channelIndexes[i] = m_channelSelectCombos[i]->currentIndex();
    }

    if (settings.recordRoot.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("保存失败"), QStringLiteral("保存路径不能为空。"));
        return;
    }

    QDir dir(settings.recordRoot);
    if (!dir.exists() && !QDir().mkpath(settings.recordRoot)) {
        QMessageBox::warning(this, QStringLiteral("保存失败"), QStringLiteral("无法创建该目录。"));
        return;
    }

    if (!AppSettings::saveStorageSettings(settings)) {
        QMessageBox::warning(this, QStringLiteral("保存失败"), QStringLiteral("配置文件写入失败。"));
        return;
    }

    QMessageBox::information(this, QStringLiteral("保存成功"), QStringLiteral("系统配置已保存。"));
    emit settingsSaved();
}

void SettingWidget::refreshCameraLabels()
{
    const QStringList cameraNames = detectedCameraNames();

    for (int i = 0; i < 4; ++i) {
        if (i < cameraNames.size()) {
            m_cameraLabels[i]->setText(cameraNames.at(i));
        } else if (i == 0) {
            m_cameraLabels[i]->setText(QStringLiteral("未检测到摄像头"));
        } else {
            m_cameraLabels[i]->setText(QStringLiteral("演示源%1").arg(i));
        }
    }
}

QStringList SettingWidget::detectedCameraNames() const
{
    QStringList cameraNames;
    const QList<QCameraDevice> devices = QMediaDevices::videoInputs();
    for (const QCameraDevice &device : devices) {
        cameraNames.append(device.description());
    }
    return cameraNames;
}
