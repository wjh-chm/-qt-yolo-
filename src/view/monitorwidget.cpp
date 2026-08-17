#include "monitorwidget.h"

#include "util/appsettings.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFontMetrics>
#include <QIcon>
#include <QMetaObject>
#include <QPen>
#include <QPixmap>

namespace
{
constexpr qint64 kMaxDetectionFrameLag = 12;
}

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
    qRegisterMetaType<DetectionBox>("DetectionBox");
    qRegisterMetaType<DetectionBoxes>("DetectionBoxes");
    qRegisterMetaType<Exception>("Exception");
    qRegisterMetaType<Video>("Video");
    qRegisterMetaType<Image>("Image");
    qRegisterMetaType<ImageList>("ImageList");

    m_tasks.fill(nullptr);
    m_detectThreads.fill(nullptr);
    m_detectTasks.fill(nullptr);
    m_latestFrameIds.fill(-1);

    initChannelConfigs();
    reloadStorageSettings();

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(10);
    mainLayout->addWidget(left_widget, 1);
    mainLayout->addWidget(right_widget, 6);

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
        lab_status->setText(QString("%1 logged in, recording enabled").arg(m_loginUser));
    }
}

bool Monitorwidget::isLoggedIn() const
{
    return !m_loginUser.isEmpty();
}

void Monitorwidget::initChannelConfigs()
{
    const QString videoRoot = QDir(QCoreApplication::applicationDirPath()).filePath("video");
    m_channelConfigs = {{
        {0, QStringLiteral("Channel 1 / camera"), true, 0, QString()},
        {1, QStringLiteral("Channel 2 / demo A"), false, -1, QDir(videoRoot).filePath("f1_seedance.mp4")},
        {2, QStringLiteral("Channel 3 / demo B"), false, -1, QDir(videoRoot).filePath("light_seedance.mp4")},
        {3, QStringLiteral("Channel 4 / demo C"), false, -1, QDir(videoRoot).filePath("hub.mp4")},
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

    QVBoxLayout *leftLayout = new QVBoxLayout(left_widget);
    QLabel *title = new QLabel(QStringLiteral("通道列表"));
    title->setObjectName("device_list");
    title->setAlignment(Qt::AlignCenter);

    device_list = new QListWidget;
    leftLayout->addWidget(title);
    leftLayout->addWidget(device_list);
    leftLayout->addStretch();

    bindDeviceList();
}

void Monitorwidget::init_rightwidget()
{
    QVBoxLayout *rightLayout = new QVBoxLayout(right_widget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(8);

    video_layout = new QStackedLayout;
    QWidget *singlePage = new QWidget;
    QWidget *quadPage = new QWidget;
    video_layout->addWidget(singlePage);
    video_layout->addWidget(quadPage);
    rightLayout->addLayout(video_layout, 8);

    QVBoxLayout *singleLayout = new QVBoxLayout(singlePage);
    lab_video = new QLabel;
    lab_video->setObjectName("video_view");
    lab_video->setMinimumSize(800, 520);
    lab_video->setScaledContents(true);
    lab_video->setAlignment(Qt::AlignCenter);
    singleLayout->addWidget(lab_video);

    QGridLayout *quadLayout = new QGridLayout(quadPage);
    quadLayout->setSpacing(8);
    for (int i = 0; i < 4; ++i) {
        lab_videos[i] = new QLabel;
        lab_videos[i]->setObjectName("video_view");
        lab_videos[i]->setMinimumSize(360, 250);
        lab_videos[i]->setScaledContents(true);
        lab_videos[i]->setAlignment(Qt::AlignCenter);
        quadLayout->addWidget(lab_videos[i], i / 2, i % 2);
        showPlaceholder(i, QStringLiteral("waiting for frame"));
    }

    lab_status = new QLabel(QStringLiteral("开始预览"));
    lab_status->setObjectName("monitor_status");
    rightLayout->addWidget(lab_status);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    btn_channel1 = new QPushButton(QIcon(":/image/w1.png"), QString());
    btn_channel4 = new QPushButton(QIcon(":/image/w4 .png"), QString());
    m_check_motionDetect = new QCheckBox(QStringLiteral("移动侦测"));
    m_check_motionDetect->setChecked(false);

    btn_channel1->setToolTip(QStringLiteral("单通道"));
    btn_channel4->setToolTip(QStringLiteral("四通道"));
    btn_channel1->setIconSize(QSize(40, 40));
    btn_channel4->setIconSize(QSize(40, 40));

    buttonLayout->addWidget(btn_channel1);
    buttonLayout->addWidget(btn_channel4);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_check_motionDetect);
    rightLayout->addLayout(buttonLayout, 1);

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
    m_detecting = (state == Qt::Checked);

    if (!m_detecting) {
        for (int i = 0; i < 4; ++i) {
            clearDetectionBoxes(i);
        }
    }

    refreshDetectionTaskEnables();
}

void Monitorwidget::applyLoadedSettings()
{
    const StorageSettings settings = AppSettings::loadStorageSettings();
    m_recordRoot = settings.recordRoot;
    m_segmentDurationSeconds = settings.intervalSeconds;

    for (int i = 0; i < 4; ++i) {
        m_channelConfigs[i].displayName = QString("Channel %1 / %2").arg(i + 1).arg(settings.channelNames[i]);
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
    if (!hasRunningTask()) {
        startChannels(4);
        swtich_to_channel1();
    }
}

QString Monitorwidget::channelName(int channelId) const
{
    if (channelId < 0 || channelId >= static_cast<int>(m_channelConfigs.size())) {
        return QStringLiteral("Unknown Channel");
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
        lab_status->setText(m_recording ? QStringLiteral("preview running, recording enabled")
                                        : QStringLiteral("preview running, preview only"));
    }

    refreshDetectionTaskEnables();
}

void Monitorwidget::startChannel(int channelId)
{
    if (channelId < 0 || channelId >= 4 || m_tasks[channelId] != nullptr) {
        return;
    }

    m_latestFrames[channelId] = QImage();
    m_latestFrameIds[channelId] = -1;
    m_latestDetectionBoxes[channelId].clear();
    showPlaceholder(channelId, QStringLiteral("waiting for frame"));

    if (m_detectThreads[channelId] == nullptr) {
        m_detectThreads[channelId] = new QThread(this);
        m_detectTasks[channelId] = new DetectTask(channelId);
        m_detectTasks[channelId]->moveToThread(m_detectThreads[channelId]);
        connect(m_detectThreads[channelId], &QThread::finished, m_detectTasks[channelId], &QObject::deleteLater);
        connect(m_detectTasks[channelId],
                &DetectTask::sendDetectionResult,
                this,
                &Monitorwidget::on_detect_result);
        m_detectThreads[channelId]->start();

        QMetaObject::invokeMethod(
            m_detectTasks[channelId],
            "setDetectEnable",
            Qt::QueuedConnection,
            Q_ARG(bool, isDetectionInputEnabled(channelId)));
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
    task->setAnomalyDurationSeconds(10);
    task->setAnomalyVideoRoot(QDir(QCoreApplication::applicationDirPath()).filePath("records/anomaly"));
    task->setAnomalyImageRoot(QDir(QCoreApplication::applicationDirPath()).filePath("feature/anomaly"));
    task->setRecordingEnabled(m_recording);

    connect(task, &MonitorTask::frameReady, this, &Monitorwidget::on_frame_ready);
    connect(task,
            &MonitorTask::frameReady,
            [detectTask = m_detectTasks[channelId]](int id, qint64 frameId, const QImage &frame) {
        if (detectTask != nullptr) {
            detectTask->submitFrame(id, frameId, frame);
        }
    });
    connect(task, &MonitorTask::statusChanged, this, &Monitorwidget::on_status_changed);
    connect(task, &MonitorTask::recordingChanged, this, &Monitorwidget::on_recording_changed);
    connect(task, &MonitorTask::anomalySessionReady, this, &Monitorwidget::on_anomaly_session_ready);

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
        m_latestFrameIds[i] = -1;
        m_latestDetectionBoxes[i].clear();
        showPlaceholder(i, QStringLiteral("not running"));
    }

    if (resetRecordingState) {
        m_recording = false;
        if (lab_status != nullptr) {
            lab_status->setText(QStringLiteral("all channels stopped"));
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
        lab_video->setText(QString("%1: %2").arg(channelName(channelId)).arg(message));
    }
}

void Monitorwidget::showFrame(QLabel *label, const QImage &frame)
{
    if (label == nullptr || frame.isNull()) {
        return;
    }

    label->setText(QString());
    label->setPixmap(QPixmap::fromImage(frame));
}

QImage Monitorwidget::composeDisplayFrame(int channelId) const
{
    if (channelId < 0 || channelId >= 4 || m_latestFrames[channelId].isNull()) {
        return QImage();
    }

    if (!m_detecting || m_latestDetectionBoxes[channelId].isEmpty()) {
        return m_latestFrames[channelId];
    }

    QImage composed = m_latestFrames[channelId].copy();
    QPainter painter(&composed);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(QPen(Qt::red, 2));
    const QFontMetrics metrics(painter.font());
    for (const DetectionBox &box : m_latestDetectionBoxes[channelId]) {
        const QRect &rect = box.rect;
        painter.drawRect(rect);

        const QString label = QString("%1 %2")
                                  .arg(box.className)
                                  .arg(box.confidence, 0, 'f', 2);
        QRect labelRect = metrics.boundingRect(label).adjusted(-4, -2, 4, 2);
        labelRect.moveTopLeft(QPoint(rect.left(), qMax(0, rect.top() - labelRect.height())));
        painter.fillRect(labelRect, QColor(255, 0, 0, 180));
        painter.setPen(Qt::white);
        painter.drawText(labelRect, Qt::AlignCenter, label);
        painter.setPen(QPen(Qt::red, 2));
    }
    return composed;
}

bool Monitorwidget::isDetectionResultExpired(int channelId, qint64 resultFrameId) const
{
    if (channelId < 0 || channelId >= 4 || resultFrameId < 0) {
        return true;
    }

    const qint64 latestFrameId = m_latestFrameIds[channelId];
    if (latestFrameId < 0) {
        return true;
    }

    return latestFrameId - resultFrameId > kMaxDetectionFrameLag;
}

bool Monitorwidget::isDetectionInputEnabled(int channelId) const
{
    if (!m_detecting || channelId < 0 || channelId >= 4) {
        return false;
    }

    if (video_layout != nullptr && video_layout->currentIndex() == 1) {
        return true;
    }

    return channelId == m_selectedChannel;
}

void Monitorwidget::refreshDetectionTaskEnables()
{
    for (int i = 0; i < 4; ++i) {
        const bool enabled = isDetectionInputEnabled(i);
        if (!enabled) {
            clearDetectionBoxes(i);
        }

        if (m_detectTasks[i] != nullptr) {
            QMetaObject::invokeMethod(
                m_detectTasks[i],
                "setDetectEnable",
                Qt::QueuedConnection,
                Q_ARG(bool, enabled));
        }
    }
}

void Monitorwidget::clearDetectionBoxes(int channelId)
{
    if (channelId < 0 || channelId >= 4) {
        return;
    }

    m_latestDetectionBoxes[channelId].clear();
    if (!m_latestFrames[channelId].isNull()) {
        showFrame(lab_videos[channelId], m_latestFrames[channelId]);
        if (channelId == m_selectedChannel) {
            showFrame(lab_video, m_latestFrames[channelId]);
        }
    }
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
    refreshDetectionTaskEnables();
    const QImage displayFrame = composeDisplayFrame(m_selectedChannel);
    if (!displayFrame.isNull()) {
        showFrame(lab_video, displayFrame);
    } else {
        showPlaceholder(m_selectedChannel, QStringLiteral("waiting for frame"));
    }
}

void Monitorwidget::swtich_to_channel4()
{
    video_layout->setCurrentIndex(1);
    refreshDetectionTaskEnables();
}

void Monitorwidget::start_single_channel()
{
    swtich_to_channel1();
}

void Monitorwidget::start_four_channel()
{
    swtich_to_channel4();
}

void Monitorwidget::select_channel(QListWidgetItem *item)
{
    if (item == nullptr) {
        return;
    }

    m_selectedChannel = item->data(Qt::UserRole).toInt();
    swtich_to_channel1();
}

void Monitorwidget::on_frame_ready(int channelId, qint64 frameId, const QImage &frame)
{
    if (channelId < 0 || channelId >= 4) {
        return;
    }

    m_latestFrames[channelId] = frame;
    m_latestFrameIds[channelId] = frameId;
    const QImage displayFrame = composeDisplayFrame(channelId);
    showFrame(lab_videos[channelId], displayFrame);
    if (channelId == m_selectedChannel) {
        showFrame(lab_video, displayFrame);
    }
}

void Monitorwidget::on_detect_result(int channelId, qint64 frameId, const DetectionBoxes &boxes)
{
    if (!m_detecting || channelId < 0 || channelId >= 4) {
        return;
    }

    if (isDetectionResultExpired(channelId, frameId)) {
        return;
    }

    m_latestDetectionBoxes[channelId] = boxes;
    if (!boxes.isEmpty() && m_tasks[channelId] != nullptr) {
        m_tasks[channelId]->notifyAnomalyDetected();
    }

    const QImage displayFrame = composeDisplayFrame(channelId);
    if (displayFrame.isNull()) {
        return;
    }

    showFrame(lab_videos[channelId], displayFrame);
    if (channelId == m_selectedChannel) {
        showFrame(lab_video, displayFrame);
    }
}

void Monitorwidget::on_status_changed(int channelId, const QString &message)
{
    if (lab_status != nullptr) {
        lab_status->setText(QString("%1: %2").arg(channelName(channelId)).arg(message));
    }

    if (message == QStringLiteral("camera open failed")
        || message == QStringLiteral("video file missing")
        || message == QStringLiteral("video file open failed")) {
        showPlaceholder(channelId, message);
    } else if (message == QStringLiteral("stopped")) {
        showPlaceholder(channelId, QStringLiteral("not running"));
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
            lab_status->setText(QString("%1: recording %2")
                                    .arg(channelName(channelId))
                                    .arg(QFileInfo(filePath).fileName()));
        }
        return;
    }

    if (!filePath.isEmpty()) {
        saveVideoRecord(channelId, filePath, startTime, endTime);
        if (lab_status != nullptr) {
            lab_status->setText(QString("%1: saved %2")
                                    .arg(channelName(channelId))
                                    .arg(QFileInfo(filePath).fileName()));
        }
    }
}

void Monitorwidget::on_anomaly_session_ready(const Exception &exception,
                                             const Video &video,
                                             const ImageList &images)
{
    if (!exception.isValid() || !video.isValid()) {
        return;
    }

    const int exceptionId = m_exceptionModel.insertException(exception);
    if (exceptionId <= 0) {
        if (lab_status != nullptr) {
            lab_status->setText(QStringLiteral("anomaly database insert failed: exception_log"));
        }
        return;
    }

    Video anomalyVideo(video.channelId(),
                       video.videoName(),
                       video.filePath(),
                       video.startTime(),
                       video.endTime(),
                       exceptionId);
    if (!m_videoModel.insertVideo(anomalyVideo) && lab_status != nullptr) {
        lab_status->setText(QStringLiteral("anomaly database insert failed: video"));
    }

    bool imageInsertFailed = false;
    for (const Image &image : images) {
        Image anomalyImage(image.channelId(),
                           image.imageName(),
                           image.imagePath(),
                           image.captureTime(),
                           QDateTime::currentDateTime(),
                           exceptionId);
        if (!m_imageModel.insertImage(anomalyImage)) {
            imageInsertFailed = true;
        }
    }

    if (lab_status != nullptr) {
        if (imageInsertFailed) {
            lab_status->setText(QStringLiteral("anomaly saved, but some image records failed"));
        } else {
            lab_status->setText(QString("%1: anomaly session saved")
                                    .arg(channelName(video.channelId() - 1)));
        }
    }
}

void Monitorwidget::reloadStorageSettings()
{
    applyLoadedSettings();
}
