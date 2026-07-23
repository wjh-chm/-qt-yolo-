#include "monitorwidget.h"

#include "../util/appsettings.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QMetaObject>
#include <QPixmap>

Monitorwidget::Monitorwidget(QWidget *parent)
    : QWidget(parent),
      left_widget(new QWidget(this)),
      right_widget(new QWidget(this)),
      device_list(nullptr),
      btn_channel1(nullptr),
      btn_channel4(nullptr),
      m_check_motionDetect(nullptr),
      lab_video(nullptr),
      lab_status(nullptr),
      video_layout(nullptr),
      m_recordRoot(QDir(QCoreApplication::applicationDirPath()).filePath("records")),
      m_segmentDurationSeconds(30),
      m_selectedChannel(0),
      m_recording(false),
      m_detecting(false)
{
    m_tasks.fill(nullptr);
    m_detectThreads.fill(nullptr);
    m_detectTasks.fill(nullptr);

    initChannelConfigs();
    reloadStorageSettings();

    QHBoxLayout *main_layout = new QHBoxLayout(this);
    main_layout->setContentsMargins(8, 8, 8, 8);
    main_layout->setSpacing(10);
    main_layout->addWidget(left_widget, 1);
    main_layout->addWidget(right_widget, 6);

    init_leftwidget();
    init_rightwidget();
    init_monitorqss();
    init_connect();
    ensurePreviewRunning();
}

Monitorwidget::~Monitorwidget()
{
    stopAllChannels();
}

void Monitorwidget::setLoginUser(const QString &username)
{
    m_loginUser = username;
    applyRecordingState();

    if (!m_loginUser.isEmpty() && lab_status != nullptr) {
        lab_status->setText(QStringLiteral("%1 已登录，已自动开启录像").arg(m_loginUser));
    }
}

bool Monitorwidget::isLoggedIn() const
{
    return !m_loginUser.isEmpty();
}

void Monitorwidget::initChannelConfigs()
{
    const QString videoRoot = QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("../video");
    m_channelConfigs = {{
        {0, QStringLiteral("通道1 / 实时摄像头"), true, 0, QString()},
        {1, QStringLiteral("通道2 / 演示视频 A"), false, -1, QDir(videoRoot).filePath("f1_seedance.mp4")},
        {2, QStringLiteral("通道3 / 演示视频 B"), false, -1, QDir(videoRoot).filePath("light_seedance.mp4")},
        {3, QStringLiteral("通道4 / 演示视频 C"), false, -1, QDir(videoRoot).filePath("mo_seedance.mp4")},
    }};
}

void Monitorwidget::bindDeviceList()
{
    if (device_list == nullptr) {
        return;
    }

    device_list->clear();
    for (const ChannelConfig &config : m_channelConfigs) {
        QListWidgetItem *item = new QListWidgetItem(config.displayName);
        item->setData(Qt::UserRole, config.channelId);
        device_list->addItem(item);
    }
    device_list->setCurrentRow(m_selectedChannel);
}

void Monitorwidget::init_leftwidget()
{
    left_widget->setObjectName("left_box");

    QVBoxLayout *left_layout = new QVBoxLayout(left_widget);
    QLabel *lab_devicelist = new QLabel(QStringLiteral("通道列表"));
    lab_devicelist->setObjectName("device_list");
    lab_devicelist->setAlignment(Qt::AlignCenter);

    device_list = new QListWidget;
    left_layout->addWidget(lab_devicelist);
    left_layout->addWidget(device_list);
    left_layout->addStretch();

    bindDeviceList();
}

void Monitorwidget::init_rightwidget()
{
    QVBoxLayout *right_layout = new QVBoxLayout(right_widget);
    right_layout->setContentsMargins(0, 0, 0, 0);
    right_layout->setSpacing(8);

    video_layout = new QStackedLayout;
    QWidget *w1 = new QWidget;
    QWidget *w4 = new QWidget;
    video_layout->addWidget(w1);
    video_layout->addWidget(w4);
    right_layout->addLayout(video_layout, 8);

    QVBoxLayout *w1_layout = new QVBoxLayout(w1);
    lab_video = new QLabel;
    lab_video->setObjectName("video_view");
    lab_video->setMinimumSize(800, 520);
    lab_video->setScaledContents(true);
    lab_video->setAlignment(Qt::AlignCenter);
    w1_layout->addWidget(lab_video);

    QGridLayout *w4_layout = new QGridLayout(w4);
    w4_layout->setSpacing(8);
    for (int i = 0; i < 4; ++i) {
        lab_videos[i] = new QLabel;
        lab_videos[i]->setObjectName("video_view");
        lab_videos[i]->setMinimumSize(360, 250);
        lab_videos[i]->setScaledContents(true);
        lab_videos[i]->setAlignment(Qt::AlignCenter);
        w4_layout->addWidget(lab_videos[i], i / 2, i % 2);
        showPlaceholder(i, QStringLiteral("等待画面"));
    }

    lab_status = new QLabel(QStringLiteral("预览启动中"));
    lab_status->setObjectName("monitor_status");
    right_layout->addWidget(lab_status);

    QHBoxLayout *btn_layout = new QHBoxLayout;
    btn_channel1 = new QPushButton(QIcon(":/image/w1.png"), "");
    btn_channel4 = new QPushButton(QIcon(":/image/w4 .png"), "");
    m_check_motionDetect = new QCheckBox(QStringLiteral("开启移动侦测"));
    m_check_motionDetect->setChecked(false);

    btn_channel1->setToolTip(QStringLiteral("单通道预览"));
    btn_channel4->setToolTip(QStringLiteral("四通道预览"));
    btn_channel1->setIconSize(QSize(40, 40));
    btn_channel4->setIconSize(QSize(40, 40));

    btn_layout->addWidget(btn_channel1);
    btn_layout->addWidget(btn_channel4);
    btn_layout->addStretch();
    btn_layout->addWidget(m_check_motionDetect);
    right_layout->addLayout(btn_layout, 1);

    video_layout->setCurrentIndex(1);
}

void Monitorwidget::init_monitorqss()
{
    QFile file(":/qss/monitor.qss");
    if (file.open(QFile::ReadOnly)) {
        setStyleSheet(file.readAll());
    }
}

void Monitorwidget::init_connect()
{
    connect(btn_channel1, &QPushButton::clicked, this, &Monitorwidget::swtich_to_channel1);
    connect(btn_channel4, &QPushButton::clicked, this, &Monitorwidget::swtich_to_channel4);
    connect(device_list, &QListWidget::itemClicked, this, &Monitorwidget::select_channel);
    connect(m_check_motionDetect, &QCheckBox::checkStateChanged, this, &Monitorwidget::slotDetectToggle);
}

void Monitorwidget::slotDetectToggle(Qt::CheckState state)
{
    const bool enable = (state == Qt::Checked);
    m_detecting = enable;

    for (int i = 0; i < 4; ++i) {
        if (m_detectTasks[i] != nullptr) {
            QMetaObject::invokeMethod(
                m_detectTasks[i],
                "setDetectEnable",
                Qt::QueuedConnection,
                Q_ARG(bool, enable));
        }
    }
}

void Monitorwidget::applyLoadedSettings()
{
    const StorageSettings settings = AppSettings::loadStorageSettings();
    m_recordRoot = settings.recordRoot;
    m_segmentDurationSeconds = settings.intervalSeconds;

    for (int i = 0; i < 4; ++i) {
        m_channelConfigs[i].displayName =
            QStringLiteral("通道%1 / %2").arg(i + 1).arg(settings.channelNames[i]);
    }

    bindDeviceList();

    for (int i = 0; i < 4; ++i) {
        if (m_tasks[i] != nullptr) {
            m_tasks[i]->setRecordRoot(m_recordRoot);
            m_tasks[i]->setSegmentDurationSeconds(m_segmentDurationSeconds);
        }
    }
}

void Monitorwidget::applyRecordingState()
{
    m_recording = isLoggedIn();

    for (MonitorTask *task : m_tasks) {
        if (task != nullptr) {
            task->setRecordingEnabled(m_recording);
        }
    }
}

void Monitorwidget::ensurePreviewRunning()
{
    if (hasRunningTask()) {
        return;
    }

    startChannels(4);
}

QString Monitorwidget::channelName(int channelId) const
{
    if (channelId < 0 || channelId >= static_cast<int>(m_channelConfigs.size())) {
        return QStringLiteral("未知通道");
    }
    return m_channelConfigs[channelId].displayName;
}

void Monitorwidget::startChannels(int channelCount)
{
    const bool shouldRecord = isLoggedIn();
    stopAllChannels(false);
    m_recording = shouldRecord;

    if (channelCount == 1) {
        startChannel(m_selectedChannel);
        video_layout->setCurrentIndex(0);
    } else {
        for (int i = 0; i < 4; ++i) {
            startChannel(i);
        }
        video_layout->setCurrentIndex(1);
    }

    if (lab_status != nullptr) {
        lab_status->setText(m_recording ? QStringLiteral("预览中，录像已自动开启")
                                        : QStringLiteral("预览中，当前为未登录只预览模式"));
    }
}

void Monitorwidget::startChannel(int channelId)
{
    if (channelId < 0 || channelId >= 4 || m_tasks[channelId] != nullptr) {
        return;
    }

    showPlaceholder(channelId, QStringLiteral("等待画面"));

    if (m_detectThreads[channelId] == nullptr) {
        m_detectThreads[channelId] = new QThread(this);
        m_detectTasks[channelId] = new DetectTask(channelId);
        m_detectTasks[channelId]->moveToThread(m_detectThreads[channelId]);
        connect(m_detectThreads[channelId], &QThread::finished, m_detectTasks[channelId], &QObject::deleteLater);
        connect(m_detectTasks[channelId],
                &DetectTask::sendDrawFrame,
                this,
                &Monitorwidget::on_detect_draw_frame);
        m_detectThreads[channelId]->start();

        QMetaObject::invokeMethod(
            m_detectTasks[channelId],
            "setDetectEnable",
            Qt::QueuedConnection,
            Q_ARG(bool, m_detecting));
    }

    const ChannelConfig &config = m_channelConfigs[channelId];
    MonitorTask *task = new MonitorTask(channelId, this);
    if (config.useCamera) {
        task->setCameraSource(config.cameraIndex);
    } else {
        task->setVideoFileSource(config.videoFilePath);
    }

    task->setRecordRoot(m_recordRoot);
    task->setSegmentDurationSeconds(m_segmentDurationSeconds);
    task->setRecordingEnabled(m_recording);
    connect(task, &MonitorTask::frameReady, this, &Monitorwidget::on_frame_ready);
    connect(task,
            &MonitorTask::frameReady,
            m_detectTasks[channelId],
            &DetectTask::recvFrame,
            Qt::QueuedConnection);
    connect(task, &MonitorTask::statusChanged, this, &Monitorwidget::on_status_changed);
    connect(task,
            &MonitorTask::recordingChanged,
            this,
            &Monitorwidget::on_recording_changed);
    m_tasks[channelId] = task;
    task->start();
}

void Monitorwidget::stopDetectChannel(int channelId)
{
    if (channelId < 0 || channelId >= 4) {
        return;
    }

    if (m_detectThreads[channelId] != nullptr) {
        m_detectThreads[channelId]->quit();
        m_detectThreads[channelId]->wait(1500);
        delete m_detectThreads[channelId];
        m_detectThreads[channelId] = nullptr;
        m_detectTasks[channelId] = nullptr;
    }
}

void Monitorwidget::stopAllChannels(bool resetRecordingState)
{
    for (int i = 0; i < 4; ++i) {
        if (m_tasks[i] != nullptr) {
            m_tasks[i]->stop();
            m_tasks[i]->wait(1500);
            delete m_tasks[i];
            m_tasks[i] = nullptr;
        }

        stopDetectChannel(i);
        m_latestFrames[i] = QImage();
        showPlaceholder(i, QStringLiteral("未启动"));
    }

    if (resetRecordingState) {
        m_recording = false;
        if (lab_status != nullptr) {
            lab_status->setText(QStringLiteral("已停止全部预览"));
        }
    }
}

void Monitorwidget::showPlaceholder(int channelId, const QString &message)
{
    if (channelId < 0 || channelId >= 4 || lab_videos[channelId] == nullptr) {
        return;
    }

    lab_videos[channelId]->setPixmap(QPixmap(":/image/nocamera.png"));
    lab_videos[channelId]->setText(message);
    if (channelId == m_selectedChannel && lab_video != nullptr) {
        lab_video->setPixmap(QPixmap(":/image/nocamera.png"));
        lab_video->setText(QStringLiteral("%1：%2").arg(channelName(channelId)).arg(message));
    }
}

void Monitorwidget::showFrame(QLabel *label, const QImage &frame)
{
    if (label == nullptr || frame.isNull()) {
        return;
    }

    label->setText("");
    label->setPixmap(QPixmap::fromImage(frame));
}

bool Monitorwidget::hasRunningTask() const
{
    for (MonitorTask *task : m_tasks) {
        if (task != nullptr) {
            return true;
        }
    }
    return false;
}

void Monitorwidget::saveVideoRecord(int channelId,
                                    const QString &filePath,
                                    const QDateTime &startTime,
                                    const QDateTime &endTime)
{
    if (filePath.isEmpty() || !startTime.isValid() || !endTime.isValid()) {
        return;
    }

    Video video(channelId + 1,
                QFileInfo(filePath).fileName(),
                filePath,
                startTime,
                endTime);
    m_videoModel.insertVideo(video);
}

void Monitorwidget::swtich_to_channel1()
{
    video_layout->setCurrentIndex(0);
    if (!m_latestFrames[m_selectedChannel].isNull()) {
        showFrame(lab_video, m_latestFrames[m_selectedChannel]);
    } else {
        showPlaceholder(m_selectedChannel, QStringLiteral("等待画面"));
    }
}

void Monitorwidget::swtich_to_channel4()
{
    video_layout->setCurrentIndex(1);
}

void Monitorwidget::start_single_channel()
{
    startChannels(1);
}

void Monitorwidget::start_four_channel()
{
    startChannels(4);
}

void Monitorwidget::select_channel(QListWidgetItem *item)
{
    if (item == nullptr) {
        return;
    }

    m_selectedChannel = item->data(Qt::UserRole).toInt();
    swtich_to_channel1();
}

void Monitorwidget::on_frame_ready(int channelId, const QImage &frame)
{
    if (channelId < 0 || channelId >= 4) {
        return;
    }

    if (m_detecting) {
        return;
    }

    m_latestFrames[channelId] = frame;
    showFrame(lab_videos[channelId], frame);
    if (channelId == m_selectedChannel) {
        showFrame(lab_video, frame);
    }
}

void Monitorwidget::on_detect_draw_frame(int channelId, const QImage &frame)
{
    if (!m_detecting || channelId < 0 || channelId >= 4 || frame.isNull()) {
        return;
    }

    m_latestFrames[channelId] = frame;
    showFrame(lab_videos[channelId], frame);
    if (channelId == m_selectedChannel) {
        showFrame(lab_video, frame);
    }
}

void Monitorwidget::on_status_changed(int channelId, const QString &message)
{
    if (lab_status != nullptr) {
        lab_status->setText(QStringLiteral("%1：%2").arg(channelName(channelId)).arg(message));
    }

    if (message == QStringLiteral("摄像头打开失败")
        || message == QStringLiteral("视频文件不存在")
        || message == QStringLiteral("视频文件打开失败")) {
        showPlaceholder(channelId, message);
    } else if (message == QStringLiteral("已停止")) {
        showPlaceholder(channelId, QStringLiteral("未启动"));
    }
}

void Monitorwidget::on_recording_changed(int channelId,
                                         bool recording,
                                         const QString &filePath,
                                         const QDateTime &startTime,
                                         const QDateTime &endTime)
{
    if (recording) {
        if (lab_status != nullptr) {
            lab_status->setText(QStringLiteral("%1：正在录像 %2")
                                    .arg(channelName(channelId))
                                    .arg(QFileInfo(filePath).fileName()));
        }
        return;
    }

    if (!filePath.isEmpty()) {
        saveVideoRecord(channelId, filePath, startTime, endTime);
        if (lab_status != nullptr) {
            lab_status->setText(QStringLiteral("%1：录像已保存 %2")
                                    .arg(channelName(channelId))
                                    .arg(QFileInfo(filePath).fileName()));
        }
    }
}

void Monitorwidget::reloadStorageSettings()
{
    applyLoadedSettings();
}
