#include "reviewwidget.h"

#include "net/clientapi.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QJsonObject>
#include <QJsonValue>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QUrl>
#include <QVBoxLayout>
#include <QVideoFrame>
#include <QVideoSink>

#include <utility>

ReviewWidget::ReviewWidget(ReviewScope scope, QWidget *parent)
    : QWidget(parent),
      m_scope(scope),
      m_recordMode(RecordMode::Video),
      m_pageSize(20),
      m_curPage(1),
      m_totalPage(1),
      channelid(0)
{
    initUI();
    m_edit_date->setDate(resolveInitialFilterDate());
    m_btn_prev->setEnabled(false);
    m_btn_next->setEnabled(false);

    connect(m_btn_next, &QPushButton::clicked, this, [this]() {
        if (m_curPage < m_totalPage) {
            loadPageData(m_curPage + 1);
        }
    });
    connect(m_btn_prev, &QPushButton::clicked, this, [this]() {
        if (m_curPage > 1) {
            loadPageData(m_curPage - 1);
        }
    });
    connect(m_edit_date, &QDateEdit::dateChanged, this, [this](const QDate &) {
        resetCurrentPreviewState();
        loadPageData(1);
    });
    connect(m_box_channel, &QComboBox::currentIndexChanged, this, [this](int) {
        resetCurrentPreviewState();
        loadPageData(1);
    });
    connect(m_list_record, &QListWidget::itemClicked, this, &ReviewWidget::itemselected);
    connect(m_btn_video_preview, &QPushButton::clicked, this, [this]() {
        switchRecordMode(RecordMode::Video);
    });
    connect(m_btn_image_preview, &QPushButton::clicked, this, [this]() {
        switchRecordMode(RecordMode::Image);
    });
    connect(btn_play, &QPushButton::clicked, this, &ReviewWidget::play);
    connect(btn_save, &QPushButton::clicked, this, &ReviewWidget::onsavedClicked);
    connect(m_detail_widget,
            &DetailWidget::requestOpenRelatedVideo,
            this,
            [this](int channelId, const QDateTime &captureTime) {
                openRelatedVideoForImage(channelId, captureTime);
            });
    connect(m_detail_widget,
            &DetailWidget::requestDeleteImages,
            this,
            [this](const QList<DetailImageItem> &images) {
                deleteImages(images);
            });
    connect(m_detail_widget,
            &DetailWidget::requestExportImages,
            this,
            [this](const QList<DetailImageItem> &images) {
                exportImages(images);
            });

    connect(m_player, &QMediaPlayer::durationChanged, this, [this](qint64 duration) {
        if (duration <= 0) {
            resetPlaybackUi();
            return;
        }

        m_slider_progress->setRange(0, static_cast<int>(duration));
        m_lab_total_time->setText(formatTime(duration));
    });
    connect(m_player, &QMediaPlayer::positionChanged, this, [this](qint64 position) {
        if (!m_slider_progress->isSliderDown()) {
            m_slider_progress->setValue(static_cast<int>(position));
        }
        m_lab_current_time->setText(formatTime(position));
    });
    connect(m_slider_progress, &QSlider::sliderReleased, this, [this]() {
        if (!m_loadedPath.isEmpty()) {
            m_player->setPosition(m_slider_progress->value());
        }
    });
    connect(box_speed, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_player->setPlaybackRate(box_speed->itemData(index).toDouble());
    });
    connect(m_player,
            &QMediaPlayer::playbackStateChanged,
            this,
            [this](QMediaPlayer::PlaybackState state) {
                btn_play->setText(state == QMediaPlayer::PlayingState ? QStringLiteral("暂停")
                                                                      : QStringLiteral("播放"));
            });
}

void ReviewWidget::setClientApi(ClientApi *api)
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
            &ClientApi::videoListFinished,
            this,
            &ReviewWidget::onVideoListFinished,
            Qt::UniqueConnection);
    connect(m_clientApi,
            &ClientApi::imageListFinished,
            this,
            &ReviewWidget::onImageListFinished,
            Qt::UniqueConnection);
    connect(m_clientApi,
            &ClientApi::relatedVideoFinished,
            this,
            &ReviewWidget::onRelatedVideoFinished,
            Qt::UniqueConnection);
    connect(m_clientApi,
            &ClientApi::deleteImagesFinished,
            this,
            &ReviewWidget::onDeleteImagesFinished,
            Qt::UniqueConnection);
    connect(m_clientApi,
            &ClientApi::insertImageFinished,
            this,
            &ReviewWidget::onInsertImageFinished,
            Qt::UniqueConnection);
}

void ReviewWidget::initUI()
{
    QHBoxLayout *mainLay = new QHBoxLayout(this);

    QWidget *leftPanel = new QWidget(this);
    QVBoxLayout *leftLay = new QVBoxLayout(leftPanel);
    leftLay->setContentsMargins(8, 8, 8, 8);
    leftLay->setSpacing(8);
    mainLay->addWidget(leftPanel, 1);

    QHBoxLayout *filterLay = new QHBoxLayout;
    filterLay->setSpacing(6);

    m_edit_date = new QDateEdit(this);
    m_edit_date->setCalendarPopup(true);
    m_edit_date->setDisplayFormat("yyyy-MM-dd");
    m_edit_date->setDate(QDate::currentDate());

    m_box_channel = new QComboBox(this);
    m_box_channel->addItem(QStringLiteral("所有通道"), 0);
    m_box_channel->addItem(QStringLiteral("通道 1"), 1);
    m_box_channel->addItem(QStringLiteral("通道 2"), 2);
    m_box_channel->addItem(QStringLiteral("通道 3"), 3);
    m_box_channel->addItem(QStringLiteral("通道 4"), 4);

    filterLay->addWidget(m_edit_date);
    filterLay->addWidget(m_box_channel);
    leftLay->addLayout(filterLay);

    m_list_record = new QListWidget(this);
    leftLay->addWidget(m_list_record, 1);

    QHBoxLayout *pageLay = new QHBoxLayout;
    m_btn_prev = new QPushButton(QStringLiteral("上一页"), this);
    m_lab_page = new QLabel(QStringLiteral("页 1 / 1"), this);
    m_btn_next = new QPushButton(QStringLiteral("下一页"), this);
    pageLay->addWidget(m_btn_prev);
    pageLay->addStretch();
    pageLay->addWidget(m_lab_page);
    pageLay->addStretch();
    pageLay->addWidget(m_btn_next);
    leftLay->addLayout(pageLay);

    m_right_panel = new QWidget(this);
    QVBoxLayout *rightLay = new QVBoxLayout(m_right_panel);
    rightLay->setContentsMargins(8, 8, 8, 8);
    rightLay->setSpacing(8);
    mainLay->addWidget(m_right_panel, 6);

    QHBoxLayout *previewBtnLay = new QHBoxLayout;
    m_btn_video_preview = new QPushButton(
        m_scope == ReviewScope::AnomalyOnly ? QStringLiteral("异常视频回看")
                                            : QStringLiteral("普通视频回放"),
        this);
    m_btn_image_preview = new QPushButton(
        m_scope == ReviewScope::AnomalyOnly ? QStringLiteral("异常图片回看")
                                            : QStringLiteral("普通图片回看"),
        this);
    previewBtnLay->addWidget(m_btn_video_preview);
    previewBtnLay->addWidget(m_btn_image_preview);
    previewBtnLay->addStretch();
    rightLay->addLayout(previewBtnLay);

    m_preview_stack = new QStackedWidget(this);
    rightLay->addWidget(m_preview_stack, 1);

    m_video_page = new QWidget(this);
    QVBoxLayout *videoLay = new QVBoxLayout(m_video_page);
    videoLay->setContentsMargins(0, 0, 0, 0);
    videoLay->setSpacing(8);

    m_video_widget = new QVideoWidget(this);
    m_video_widget->setMinimumSize(900, 600);
    videoLay->addWidget(m_video_widget, 1);

    QHBoxLayout *progressLay = new QHBoxLayout;
    m_lab_current_time = new QLabel("00:00", this);
    m_slider_progress = new QSlider(Qt::Horizontal, this);
    m_lab_total_time = new QLabel("00:00", this);
    progressLay->addWidget(m_lab_current_time);
    progressLay->addWidget(m_slider_progress, 1);
    progressLay->addWidget(m_lab_total_time);
    videoLay->addLayout(progressLay);

    QHBoxLayout *controlLay = new QHBoxLayout;
    btn_play = new QPushButton(QStringLiteral("播放"), this);
    btn_save = new QPushButton(QStringLiteral("截图"), this);
    box_speed = new QComboBox(this);
    box_speed->addItem("1.0x", 1.0);
    box_speed->addItem("0.5x", 0.5);
    box_speed->addItem("2.0x", 2.0);
    box_speed->setCurrentIndex(0);
    controlLay->addStretch();
    controlLay->addWidget(btn_play);
    controlLay->addWidget(btn_save);
    controlLay->addWidget(box_speed);
    controlLay->addStretch();
    videoLay->addLayout(controlLay);

    m_image_page = new QWidget(this);
    QVBoxLayout *imageLay = new QVBoxLayout(m_image_page);
    imageLay->setContentsMargins(0, 0, 0, 0);
    m_detail_widget = new DetailWidget(this);
    imageLay->addWidget(m_detail_widget, 1);

    m_preview_stack->addWidget(m_video_page);
    m_preview_stack->addWidget(m_image_page);
    m_preview_stack->setCurrentWidget(m_video_page);

    m_player = new QMediaPlayer(this);
    m_player->setVideoOutput(m_video_widget);
    if (m_video_widget->videoSink() != nullptr) {
        connect(m_video_widget->videoSink(),
                &QVideoSink::videoFrameChanged,
                this,
                [this](const QVideoFrame &frame) {
                    if (!frame.isValid()) {
                        return;
                    }

                    const QImage image = frame.toImage();
                    if (!image.isNull()) {
                        m_lastVideoFrame = image;
                    }
                });
    }

    resetPlaybackUi();
}

void ReviewWidget::previewMedia(const QString &path)
{
    const QFileInfo fileInfo(path);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("文件不存在或不是有效文件"));
        return;
    }

    const QString suffix = fileInfo.suffix().toLower();
    if (isImageFile(suffix)) {
        m_preview_stack->setCurrentWidget(m_image_page);
        m_detail_widget->showPreviewForPath(path);
        return;
    }

    if (isVideoFile(suffix)) {
        m_preview_stack->setCurrentWidget(m_video_page);
        return;
    }

    m_preview_stack->setCurrentWidget(m_image_page);
    QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("暂不支持该文件类型"));
}

void ReviewWidget::play()
{
    if (currentPath.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先选择一条记录"));
        return;
    }

    const QFileInfo fileInfo(currentPath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("当前文件不存在"));
        return;
    }

    if (!isVideoFile(fileInfo.suffix().toLower())) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("当前选中项不是视频文件"));
        return;
    }

    if (m_loadedPath != currentPath) {
        m_preview_stack->setCurrentWidget(m_video_page);
        m_player->setSource(QUrl::fromLocalFile(currentPath));
        m_loadedPath = currentPath;
        m_player->play();
        return;
    }

    if (m_player->playbackState() == QMediaPlayer::PlayingState) {
        m_player->pause();
    } else {
        m_player->play();
    }
}

void ReviewWidget::itemselected(QListWidgetItem *item)
{
    if (item == nullptr) {
        return;
    }

    const QString mediaPath = item->data(Qt::UserRole).toString();
    if (mediaPath.isEmpty()) {
        return;
    }

    channelid = item->data(Qt::UserRole + 2).toInt();
    currentPath = mediaPath;
    m_player->stop();
    m_loadedPath.clear();
    m_lastVideoFrame = QImage();
    resetPlaybackUi();
    if (m_recordMode == RecordMode::Image) {
        m_preview_stack->setCurrentWidget(m_image_page);
        m_detail_widget->showPreviewForPath(currentPath);
        return;
    }

    previewMedia(currentPath);
}

void ReviewWidget::onsavedClicked()
{
    if (currentPath.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("当前没有可截图的视频"));
        return;
    }
    if (m_preview_stack->currentWidget() != m_video_page) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("当前不是视频预览页面"));
        return;
    }
    if (m_lastVideoFrame.isNull()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("当前没有可截图的视频帧"));
        return;
    }

    const QString saveRoot = QDir(QCoreApplication::applicationDirPath()).filePath("feature/handle");
    QDir dir(saveRoot);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    const QString currentTime = QDateTime::currentDateTime().toString("yyyyMMddHHmmss");
    const QString fileNameTemplate = QString("手动_通道%1_%2.jpg").arg(channelid).arg(currentTime);
    const QString fullPath = QDir(saveRoot).filePath(fileNameTemplate);
    QString fileName =
        QFileDialog::getSaveFileName(this, tr("保存截图"), fullPath, tr("Images (*.jpg)"));

    if (fileName.isEmpty()) {
        return;
    }

    if (!fileName.endsWith(".jpg", Qt::CaseInsensitive)) {
        fileName += ".jpg";
    }

    const QImage frame = m_lastVideoFrame.copy();
    if (!frame.save(fileName)) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("截图保存失败"));
        return;
    }

    insertFeatureImageRecord(fileName);
    QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("截图保存成功"));
}

void ReviewWidget::refreshData()
{
    resetCurrentPreviewState();
    m_edit_date->setDate(resolveInitialFilterDate());
    loadPageData(1);
}

void ReviewWidget::loadPageData(int page)
{
    if (m_recordMode == RecordMode::Video) {
        loadVideoPageData(page);
    } else {
        loadImagePageData(page);
    }
}

QString ReviewWidget::formatTime(qint64 ms) const
{
    const int totalSeconds = static_cast<int>(ms / 1000);
    const int hours = totalSeconds / 3600;
    const int minutes = (totalSeconds % 3600) / 60;
    const int seconds = totalSeconds % 60;

    if (hours > 0) {
        return QString("%1:%2:%3")
            .arg(hours, 2, 10, QChar('0'))
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    }

    return QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
}

bool ReviewWidget::isImageFile(const QString &suffix) const
{
    return suffix == "jpg" || suffix == "jpeg" || suffix == "png" || suffix == "bmp";
}

bool ReviewWidget::isVideoFile(const QString &suffix) const
{
    return suffix == "avi" || suffix == "mp4" || suffix == "mkv" || suffix == "mov";
}

void ReviewWidget::resetPlaybackUi()
{
    m_lab_current_time->setText("00:00");
    m_lab_total_time->setText("00:00");
    m_slider_progress->setRange(0, 0);
    m_slider_progress->setValue(0);
    btn_play->setText(QStringLiteral("播放"));
}

int ReviewWidget::selectedChannelFilter() const
{
    return m_box_channel->currentData().toInt();
}

QDateTime ReviewWidget::selectedDateStart() const
{
    return QDateTime(m_edit_date->date(), QTime(0, 0, 0));
}

QDateTime ReviewWidget::selectedDateEnd() const
{
    return selectedDateStart().addDays(1);
}

void ReviewWidget::resetCurrentPreviewState()
{
    currentPath.clear();
    m_loadedPath.clear();
    m_lastVideoFrame = QImage();
    m_player->stop();
    m_detail_widget->clear();
    resetPlaybackUi();
}

QDate ReviewWidget::resolveInitialFilterDate() const
{
    return QDate::currentDate();
}

void ReviewWidget::switchRecordMode(RecordMode mode)
{
    if (m_recordMode == mode) {
        m_preview_stack->setCurrentWidget(mode == RecordMode::Video ? m_video_page : m_image_page);
        return;
    }

    m_recordMode = mode;
    resetCurrentPreviewState();
    m_edit_date->setDate(resolveInitialFilterDate());
    m_preview_stack->setCurrentWidget(mode == RecordMode::Video ? m_video_page : m_image_page);
    loadPageData(1);
}

void ReviewWidget::loadVideoPageData(int page)
{
    if (m_clientApi == nullptr || !m_clientApi->isConnected()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("服务端未连接"));
        return;
    }

    m_list_record->clear();
    m_detail_widget->clear();
    m_curPage = qMax(1, page);
    m_totalPage = qMax(1, m_curPage);
    m_lab_page->setText(QStringLiteral("加载中..."));
    m_btn_prev->setEnabled(false);
    m_btn_next->setEnabled(false);

    const QString scope = isAnomalyScope() ? QStringLiteral("anomaly") : QStringLiteral("normal");
    const QString date = m_edit_date->date().toString("yyyy-MM-dd");
    m_waitingVideoList = true;
    m_videoListRequestId = m_clientApi->queryVideoList(scope, date, selectedChannelFilter(), m_curPage, m_pageSize);
}

void ReviewWidget::loadImagePageData(int page)
{
    if (m_clientApi == nullptr || !m_clientApi->isConnected()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("服务端未连接"));
        return;
    }

    m_list_record->clear();
    m_detail_widget->clear();
    m_curPage = qMax(1, page);
    m_totalPage = qMax(1, m_curPage);
    m_lab_page->setText(QStringLiteral("加载中..."));
    m_btn_prev->setEnabled(false);
    m_btn_next->setEnabled(false);

    const QString scope = isAnomalyScope() ? QStringLiteral("anomaly") : QStringLiteral("normal");
    const QString date = m_edit_date->date().toString("yyyy-MM-dd");
    m_waitingImageList = true;
    m_imageListRequestId = m_clientApi->queryImageList(scope, date, selectedChannelFilter(), m_curPage, m_pageSize);
}

void ReviewWidget::onVideoListFinished(qint64 requestId,bool success,
                                       const QJsonArray &list,
                                       int totalCount,
                                       const QString &message)
{
    if (!m_waitingVideoList || requestId != m_videoListRequestId) {
        return;
    }

    m_waitingVideoList = false;
    m_videoListRequestId = 0;
    if (m_recordMode != RecordMode::Video) {
        return;
    }

    m_list_record->clear();

    if (!success) {
        showEmptyPlaceholder(message.isEmpty() ? QStringLiteral("视频查询失败") : message);
        m_totalPage = qMax(1, m_curPage);
        m_lab_page->setText(QString("页 %1 / %2").arg(m_curPage).arg(m_totalPage));
        m_btn_prev->setEnabled(m_curPage > 1);
        m_btn_next->setEnabled(false);
        return;
    }

    for (const QJsonValue &value : list) {
        const QJsonObject record = value.toObject();
        const int id = record.value("id").toInt();
        const int channelId = record.value("channel_id").toInt();
        const QString videoName = record.value("video_name").toString();
        const QString videoPath = record.value("video_path").toString();
        const QString startTimeText = record.value("start_time").toString();
        const QString endTimeText = record.value("end_time").toString();

        const QDateTime startTime =
            QDateTime::fromString(startTimeText, "yyyy-MM-dd HH:mm:ss");
        const QString displayTime =
            startTime.isValid() ? startTime.toString("yyyyMMddHHmmss") : startTimeText;

        QListWidgetItem *item =
            new QListWidgetItem(QString("通道 %1  %2").arg(channelId).arg(displayTime));
        item->setData(Qt::UserRole, videoPath);
        item->setData(Qt::UserRole + 1, id);
        item->setData(Qt::UserRole + 2, channelId);
        item->setData(Qt::UserRole + 3, videoName);
        item->setData(Qt::UserRole + 4, endTimeText);
        m_list_record->addItem(item);
    }

    if (list.isEmpty()) {
        showEmptyPlaceholder(QString("%1下暂无视频").arg(scopeTitle()));
    }

    m_totalPage = qMax(1, (totalCount + m_pageSize - 1) / m_pageSize);
    m_lab_page->setText(QString("页 %1 / %2").arg(m_curPage).arg(m_totalPage));
    m_btn_prev->setEnabled(m_curPage > 1);
    m_btn_next->setEnabled(m_curPage < m_totalPage);
}

void ReviewWidget::onImageListFinished(qint64 requestId,bool success,
                                       const QJsonArray &list,
                                       int totalCount,
                                       const QString &message)
{
    if (!m_waitingImageList || requestId != m_imageListRequestId) {
        return;
    }

    m_waitingImageList = false;
    m_imageListRequestId = 0;
    if (m_recordMode != RecordMode::Image) {
        return;
    }

    m_list_record->clear();
    m_detail_widget->clear();

    if (!success) {
        showEmptyPlaceholder(message.isEmpty() ? QStringLiteral("图片查询失败") : message);
        m_totalPage = qMax(1, m_curPage);
        m_lab_page->setText(QString("页 %1 / %2").arg(m_curPage).arg(m_totalPage));
        m_btn_prev->setEnabled(m_curPage > 1);
        m_btn_next->setEnabled(false);
        return;
    }

    QList<DetailImageItem> images;
    for (const QJsonValue &value : list) {
        const QJsonObject image = value.toObject();
        DetailImageItem imageItem;
        imageItem.imageId = image.value("id").toInt();
        imageItem.channelId = image.value("channel_id").toInt();
        imageItem.imageName = image.value("image_name").toString();
        imageItem.imagePath = image.value("image_path").toString();
        imageItem.captureTime =
            QDateTime::fromString(image.value("capture_time").toString(),
                                  "yyyy-MM-dd HH:mm:ss");
        const QJsonValue exceptionValue = image.value("exception_id");
        imageItem.exceptionId = exceptionValue.isNull() || exceptionValue.isUndefined()
                                    ? -1
                                    : exceptionValue.toInt(-1);
        images.append(imageItem);

        QListWidgetItem *item = new QListWidgetItem(imageItem.imageName);
        item->setData(Qt::UserRole, imageItem.imagePath);
        item->setData(Qt::UserRole + 1, imageItem.imageId);
        item->setData(Qt::UserRole + 2, imageItem.channelId);
        item->setData(Qt::UserRole + 3,
                      imageItem.captureTime.toString("yyyy-MM-dd HH:mm:ss"));
        m_list_record->addItem(item);
    }

    m_detail_widget->setImages(images);
    if (list.isEmpty()) {
        showEmptyPlaceholder(QString("%1下暂无图片").arg(scopeTitle()));
    }

    m_totalPage = qMax(1, (totalCount + m_pageSize - 1) / m_pageSize);
    m_lab_page->setText(QString("页 %1 / %2").arg(m_curPage).arg(m_totalPage));
    m_btn_prev->setEnabled(m_curPage > 1);
    m_btn_next->setEnabled(m_curPage < m_totalPage);
}

void ReviewWidget::insertFeatureImageRecord(const QString &filePath)
{
    if (m_clientApi == nullptr || !m_clientApi->isConnected()) {
        QMessageBox::warning(this,
                             QStringLiteral("提示"),
                             QStringLiteral("截图已保存，但服务端未连接，图片记录未入库"));
        return;
    }

    const QFileInfo fileInfo(filePath);
    const QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    m_waitingInsertImage = true;
    m_insertImageRequestId = m_clientApi->insertImageRecord(channelid,
                                                            fileInfo.fileName(),
                                                            filePath,
                                                            now,
                                                            now,
                                                            -1);
}

void ReviewWidget::onInsertImageFinished(qint64 requestId,bool success, int, const QString &message)
{
    if (!m_waitingInsertImage || requestId != m_insertImageRequestId) {
        return;
    }

    m_waitingInsertImage = false;
    m_insertImageRequestId = 0;

    if (!success) {
        QMessageBox::warning(this,
                             QStringLiteral("提示"),
                             message.isEmpty() ? QStringLiteral("截图已保存，但图片记录入库失败")
                                               : message);
    }
}

void ReviewWidget::showEmptyPlaceholder(const QString &text)
{
    QListWidgetItem *item = new QListWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
    m_list_record->addItem(item);
}

QString ReviewWidget::scopeTitle() const
{
    return m_scope == ReviewScope::AnomalyOnly ? QStringLiteral("当前异常筛选条件")
                                               : QStringLiteral("当前普通筛选条件");
}

bool ReviewWidget::isAnomalyScope() const
{
    return m_scope == ReviewScope::AnomalyOnly;
}

void ReviewWidget::openRelatedVideoForImage(int channelId, const QDateTime &captureTime)
{
    if (!captureTime.isValid()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("当前图片没有有效抓拍时间"));
        return;
    }

    if (m_clientApi == nullptr || !m_clientApi->isConnected()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("服务端未连接，无法查询关联视频"));
        return;
    }

    const QString scope = isAnomalyScope() ? QStringLiteral("anomaly") : QStringLiteral("normal");
    m_waitingRelatedVideo = true;
    m_pendingRelatedCaptureTime = captureTime;
    m_relatedVideoRequestId = m_clientApi->findRelatedVideo(scope,
                                                            channelId,
                                                            captureTime.toString("yyyy-MM-dd HH:mm:ss"));
}

void ReviewWidget::onRelatedVideoFinished(qint64 requestId,bool success,
                                          const QJsonObject &record,
                                          const QString &message)
{
    if (!m_waitingRelatedVideo || requestId != m_relatedVideoRequestId) {
        return;
    }

    m_waitingRelatedVideo = false;
    m_relatedVideoRequestId = 0;
    if (!success || record.isEmpty()) {
        QMessageBox::information(this,
                                 QStringLiteral("提示"),
                                 message.isEmpty() ? QStringLiteral("未查询到该图片对应的视频片段")
                                                   : message);
        return;
    }

    const int channelId = record.value("channel_id").toInt();
    const QString videoPath = record.value("video_path").toString();
    const QString startTimeText = record.value("start_time").toString();
    const QDateTime startTime =
        QDateTime::fromString(startTimeText, "yyyy-MM-dd HH:mm:ss");

    if (videoPath.isEmpty() || !startTime.isValid()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("关联视频数据不完整"));
        return;
    }

    {
        const QSignalBlocker blockDate(m_edit_date);
        const QSignalBlocker blockChannel(m_box_channel);
        m_edit_date->setDate(m_pendingRelatedCaptureTime.date());
        const int targetIndex = m_box_channel->findData(channelId);
        if (targetIndex >= 0) {
            m_box_channel->setCurrentIndex(targetIndex);
        }
    }

    m_recordMode = RecordMode::Video;
    resetCurrentPreviewState();
    m_preview_stack->setCurrentWidget(m_video_page);
    channelid = channelId;
    currentPath = videoPath;
    loadPageData(1);
    openVideoAtTimestamp(videoPath, startTime, m_pendingRelatedCaptureTime);
}

void ReviewWidget::openVideoAtTimestamp(const QString &videoPath,
                                        const QDateTime &startTime,
                                        const QDateTime &captureTime)
{
    const QFileInfo fileInfo(videoPath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("关联视频文件不存在"));
        return;
    }

    const qint64 seekPosition = qMax<qint64>(0, startTime.msecsTo(captureTime));
    m_preview_stack->setCurrentWidget(m_video_page);
    m_player->stop();
    m_loadedPath = videoPath;

    QMetaObject::Connection *connection = new QMetaObject::Connection;
    *connection = connect(m_player,
                          &QMediaPlayer::mediaStatusChanged,
                          this,
                          [this, seekPosition, connection](QMediaPlayer::MediaStatus status) {
                              if (status == QMediaPlayer::InvalidMedia
                                  || status == QMediaPlayer::NoMedia) {
                                  disconnect(*connection);
                                  delete connection;
                                  return;
                              }

                              if (status != QMediaPlayer::LoadedMedia
                                  && status != QMediaPlayer::BufferedMedia) {
                                  return;
                              }

                              m_player->setPosition(seekPosition);
                              m_player->pause();
                              disconnect(*connection);
                              delete connection;
                          });

    m_player->setSource(QUrl::fromLocalFile(videoPath));
}

void ReviewWidget::deleteImages(const QList<DetailImageItem> &images)
{
    if (images.isEmpty()) {
        return;
    }

    const QString message =
        m_scope == ReviewScope::AnomalyOnly
            ? QStringLiteral("确定删除选中的异常图片吗？关联日志和视频将保留。")
            : QStringLiteral("确定删除选中的图片吗？");

    if (QMessageBox::question(this, QStringLiteral("确认删除"), message) != QMessageBox::Yes) {
        return;
    }

    QList<int> imageIds;
    for (const DetailImageItem &image : images) {
        if (image.imageId > 0) {
            imageIds.append(image.imageId);
        }
    }

    if (imageIds.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("选中图片缺少数据库 id"));
        return;
    }

    if (m_clientApi == nullptr || !m_clientApi->isConnected()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("服务端未连接，图片数据库记录未删除"));
        return;
    }

    m_pendingDeleteImages = images;
    m_waitingDeleteImages = true;
    m_deleteImagesRequestId = m_clientApi->deleteImagesByIds(imageIds);
}

void ReviewWidget::onDeleteImagesFinished(qint64 requestId,bool success, const QString &message)
{
    if (!m_waitingDeleteImages || requestId != m_deleteImagesRequestId) {
        return;
    }

    m_waitingDeleteImages = false;
    m_deleteImagesRequestId = 0;

    if (!success) {
        m_pendingDeleteImages.clear();
        QMessageBox::warning(this,
                             QStringLiteral("提示"),
                             message.isEmpty() ? QStringLiteral("图片数据库记录删除失败") : message);
        return;
    }

    int fileFailedCount = 0;
    for (const DetailImageItem &image : std::as_const(m_pendingDeleteImages)) {
        if (image.imagePath.isEmpty()) {
            continue;
        }

        if (QFileInfo::exists(image.imagePath) && !QFile::remove(image.imagePath)) {
            ++fileFailedCount;
        }
    }

    m_pendingDeleteImages.clear();
    currentPath.clear();
    loadPageData(m_curPage);

    if (fileFailedCount > 0) {
        QMessageBox::warning(this,
                             QStringLiteral("提示"),
                             QStringLiteral("数据库已删除，但有 %1 个图片文件删除失败").arg(fileFailedCount));
        return;
    }

    QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("图片删除成功"));
}

void ReviewWidget::exportImages(const QList<DetailImageItem> &images)
{
    if (images.isEmpty()) {
        return;
    }

    const QString targetDir =
        QFileDialog::getExistingDirectory(this, QStringLiteral("选择导出目录"));
    if (targetDir.isEmpty()) {
        return;
    }

    int successCount = 0;
    int failedCount = 0;
    for (const DetailImageItem &image : images) {
        const QFileInfo sourceInfo(image.imagePath);
        if (!sourceInfo.exists() || !sourceInfo.isFile()) {
            ++failedCount;
            continue;
        }

        const QString exportName = image.imageName.isEmpty() ? sourceInfo.fileName() : image.imageName;
        const QString targetPath = uniqueExportPath(targetDir, exportName);
        if (QFile::copy(image.imagePath, targetPath)) {
            ++successCount;
        } else {
            ++failedCount;
        }
    }

    QMessageBox::information(this,
                             QStringLiteral("导出完成"),
                             QStringLiteral("成功导出 %1 张，失败 %2 张")
                                 .arg(successCount)
                                 .arg(failedCount));
}

QString ReviewWidget::uniqueExportPath(const QString &targetDir, const QString &fileName) const
{
    const QDir dir(targetDir);
    QFileInfo fileInfo(fileName);
    QString baseName = fileInfo.completeBaseName();
    QString suffix = fileInfo.suffix();

    if (baseName.isEmpty()) {
        baseName = QStringLiteral("image");
    }
    if (suffix.isEmpty()) {
        suffix = QStringLiteral("jpg");
    }

    QString candidate = dir.filePath(QString("%1.%2").arg(baseName, suffix));
    int index = 1;
    while (QFileInfo::exists(candidate)) {
        candidate = dir.filePath(QString("%1_%2.%3").arg(baseName).arg(index).arg(suffix));
        ++index;
    }

    return candidate;
}
