#include "reviewwidget.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPixmap>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUrl>
#include <QVBoxLayout>
#include <QVideoFrame>
#include <QVideoSink>

ReviewWidget::ReviewWidget(QWidget *parent)
    : QWidget(parent),
      m_recordMode(RecordMode::Video),
      m_pageSize(20),
      m_curPage(1),
      m_totalPage(1),
      channelid(0)
{
    initUI();
    m_edit_date->setDate(resolveInitialFilterDate());
    loadPageData(m_curPage);

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

void ReviewWidget::initUI()
{
    QHBoxLayout *mainLay = new QHBoxLayout;
    setLayout(mainLay);

    QWidget *leftPanel = new QWidget;
    QVBoxLayout *leftLay = new QVBoxLayout(leftPanel);
    leftLay->setContentsMargins(8, 8, 8, 8);
    leftLay->setSpacing(8);
    mainLay->addWidget(leftPanel, 1);

    QHBoxLayout *filterLay = new QHBoxLayout;
    filterLay->setSpacing(6);

    m_edit_date = new QDateEdit;
    m_edit_date->setCalendarPopup(true);
    m_edit_date->setDisplayFormat("yyyy-MM-dd");
    m_edit_date->setDate(QDate::currentDate());

    m_box_channel = new QComboBox;
    m_box_channel->addItem(QStringLiteral("全部通道"), 0);
    m_box_channel->addItem(QStringLiteral("通道1"), 1);
    m_box_channel->addItem(QStringLiteral("通道2"), 2);
    m_box_channel->addItem(QStringLiteral("通道3"), 3);
    m_box_channel->addItem(QStringLiteral("通道4"), 4);

    filterLay->addWidget(m_edit_date);
    filterLay->addWidget(m_box_channel);
    leftLay->addLayout(filterLay);

    m_list_record = new QListWidget;
    leftLay->addWidget(m_list_record, 1);

    QHBoxLayout *pageLay = new QHBoxLayout;
    m_btn_prev = new QPushButton(QStringLiteral("上一页"));
    m_lab_page = new QLabel(QStringLiteral("第 1 页 / 共 1 页"));
    m_btn_next = new QPushButton(QStringLiteral("下一页"));
    pageLay->addWidget(m_btn_prev);
    pageLay->addStretch();
    pageLay->addWidget(m_lab_page);
    pageLay->addStretch();
    pageLay->addWidget(m_btn_next);
    leftLay->addLayout(pageLay);

    m_right_panel = new QWidget;
    QVBoxLayout *rightLay = new QVBoxLayout(m_right_panel);
    rightLay->setContentsMargins(8, 8, 8, 8);
    rightLay->setSpacing(8);
    mainLay->addWidget(m_right_panel, 6);

    QHBoxLayout *previewBtnLay = new QHBoxLayout;
    m_btn_video_preview = new QPushButton(QStringLiteral("视频预览"));
    m_btn_image_preview = new QPushButton(QStringLiteral("图片预览"));
    previewBtnLay->addWidget(m_btn_video_preview);
    previewBtnLay->addWidget(m_btn_image_preview);
    previewBtnLay->addStretch();
    rightLay->addLayout(previewBtnLay);

    m_preview_stack = new QStackedWidget;
    rightLay->addWidget(m_preview_stack, 1);

    m_video_page = new QWidget;
    QVBoxLayout *videoLay = new QVBoxLayout(m_video_page);
    videoLay->setContentsMargins(0, 0, 0, 0);
    videoLay->setSpacing(8);

    m_video_widget = new QVideoWidget;
    m_video_widget->setMinimumSize(900, 600);
    videoLay->addWidget(m_video_widget, 1);

    QHBoxLayout *progressLay = new QHBoxLayout;
    m_lab_current_time = new QLabel("00:00");
    m_slider_progress = new QSlider(Qt::Horizontal);
    m_lab_total_time = new QLabel("00:00");
    progressLay->addWidget(m_lab_current_time);
    progressLay->addWidget(m_slider_progress, 1);
    progressLay->addWidget(m_lab_total_time);
    videoLay->addLayout(progressLay);

    QHBoxLayout *controlLay = new QHBoxLayout;
    btn_play = new QPushButton(QStringLiteral("播放"));
    btn_save = new QPushButton(QStringLiteral("截图"));
    box_speed = new QComboBox;
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

    m_image_page = new QWidget;
    QVBoxLayout *imageLay = new QVBoxLayout(m_image_page);
    imageLay->setContentsMargins(0, 0, 0, 0);
    m_image_label = new QLabel(QStringLiteral("暂无图片预览"));
    m_image_label->setAlignment(Qt::AlignCenter);
    m_image_label->setMinimumSize(900, 600);
    m_image_label->setScaledContents(true);
    imageLay->addWidget(m_image_label, 1);

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
    QFileInfo fileInfo(path);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        m_preview_stack->setCurrentWidget(m_image_page);
        m_image_label->setText(QStringLiteral("文件不存在或不是有效文件"));
        m_image_label->setPixmap(QPixmap());
        return;
    }

    const QString suffix = fileInfo.suffix().toLower();
    if (isImageFile(suffix)) {
        QPixmap pixmap(path);
        m_preview_stack->setCurrentWidget(m_image_page);
        if (pixmap.isNull()) {
            m_image_label->setText(QStringLiteral("图片加载失败"));
            m_image_label->setPixmap(QPixmap());
            return;
        }

        m_image_label->setText(QString());
        m_image_label->setPixmap(pixmap);
        return;
    }

    if (isVideoFile(suffix)) {
        m_preview_stack->setCurrentWidget(m_video_page);
        return;
    }

    m_preview_stack->setCurrentWidget(m_image_page);
    m_image_label->setText(QStringLiteral("暂不支持该文件类型"));
    m_image_label->setPixmap(QPixmap());
}

void ReviewWidget::play()
{
    if (currentPath.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先选择一条录像记录"));
        return;
    }

    QFileInfo fileInfo(currentPath);
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

    const QString currenttime = QDateTime::currentDateTime().toString("yyyyMMddHHmmss");
    const QString filename = QString("手动_通道%1_%2.jpg").arg(channelid).arg(currenttime);
    const QString fullPath = QDir(saveRoot).filePath(filename);
    QString fileName = QFileDialog::getSaveFileName(
        this, tr("保存截图"), fullPath, tr("Images (*.jpg)"));

    if (fileName.isEmpty()) {
        return;
    }

    if (!fileName.endsWith(".jpg", Qt::CaseInsensitive)) {
        fileName += ".jpg";
    }

    const QImage frame = m_lastVideoFrame.copy();
    if (!frame.save(fileName)) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("截图失败"));
        return;
    }

    insertFeatureImageRecord(fileName);
    QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("截图保存成功"));
}

int ReviewWidget::getTotalCount()
{
    return m_recordMode == RecordMode::Video ? getVideoTotalCount() : getImageTotalCount();
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
    m_image_label->setText(QStringLiteral("暂无图片预览"));
    m_image_label->setPixmap(QPixmap());
    resetPlaybackUi();
}

QDate ReviewWidget::resolveInitialFilterDate() const
{
    QSqlDatabase db = DbConn::getInstence()->getDb();
    if (!db.isOpen()) {
        return QDate::currentDate();
    }

    QSqlQuery query(db);
    const QString sql = m_recordMode == RecordMode::Video
        ? QStringLiteral("SELECT MAX(start_time) FROM video")
        : QStringLiteral("SELECT MAX(capture_time) FROM feature_image");
    if (!query.exec(sql)) {
        return QDate::currentDate();
    }

    if (!query.next()) {
        return QDate::currentDate();
    }

    const QDateTime latestTime = query.value(0).toDateTime();
    return latestTime.isValid() ? latestTime.date() : QDate::currentDate();
}

void ReviewWidget::switchRecordMode(RecordMode mode)
{
    if (m_recordMode == mode) {
        if (mode == RecordMode::Video) {
            m_preview_stack->setCurrentWidget(m_video_page);
        } else {
            m_preview_stack->setCurrentWidget(m_image_page);
        }
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
    QSqlDatabase db = DbConn::getInstence()->getDb();
    if (!db.isOpen()) {
        QMessageBox::critical(this, QStringLiteral("错误"), QStringLiteral("数据库连接失败"));
        return;
    }

    m_list_record->clear();

    const int total = getVideoTotalCount();
    m_totalPage = qMax(1, (total + m_pageSize - 1) / m_pageSize);
    m_curPage = page;
    m_lab_page->setText(QStringLiteral("第 %1 页 / 共 %2 页").arg(m_curPage).arg(m_totalPage));
    m_btn_prev->setEnabled(m_curPage > 1);
    m_btn_next->setEnabled(m_curPage < m_totalPage);

    const int offset = (m_curPage - 1) * m_pageSize;
    QSqlQuery query(db);
    QString sql = "SELECT id, channel_id, video_name, start_time, end_time, video_path "
                  "FROM video WHERE start_time >= ? AND start_time < ?";
    if (selectedChannelFilter() > 0) {
        sql += " AND channel_id = ?";
    }
    sql += QString(" ORDER BY start_time DESC LIMIT %1, %2").arg(offset).arg(m_pageSize);

    query.prepare(sql);
    query.addBindValue(selectedDateStart().toString("yyyy-MM-dd HH:mm:ss"));
    query.addBindValue(selectedDateEnd().toString("yyyy-MM-dd HH:mm:ss"));
    if (selectedChannelFilter() > 0) {
        query.addBindValue(selectedChannelFilter());
    }

    if (!query.exec()) {
        QMessageBox::critical(this, QStringLiteral("查询失败"), query.lastError().text());
        return;
    }

    bool hasResult = false;
    while (query.next()) {
        hasResult = true;
        const QDateTime startTime = query.value("start_time").toDateTime();
        const int currentChannelId = query.value("channel_id").toInt();
        const QString text = QString("通道%1  %2")
                                 .arg(currentChannelId)
                                 .arg(startTime.toString("yyyyMMddHHmmss"));

        QListWidgetItem *item = new QListWidgetItem(text);
        item->setData(Qt::UserRole, query.value("video_path").toString());
        item->setData(Qt::UserRole + 1, query.value("id").toInt());
        item->setData(Qt::UserRole + 2, currentChannelId);
        item->setData(Qt::UserRole + 3, query.value("video_name").toString());
        item->setData(Qt::UserRole + 4, query.value("end_time").toString());
        m_list_record->addItem(item);
    }

    if (!hasResult) {
        showEmptyPlaceholder(QStringLiteral("当前筛选条件下暂无录像"));
    }
}

void ReviewWidget::loadImagePageData(int page)
{
    QSqlDatabase db = DbConn::getInstence()->getDb();
    if (!db.isOpen()) {
        QMessageBox::critical(this, QStringLiteral("错误"), QStringLiteral("数据库连接失败"));
        return;
    }

    m_list_record->clear();

    const int total = getImageTotalCount();
    m_totalPage = qMax(1, (total + m_pageSize - 1) / m_pageSize);
    m_curPage = page;
    m_lab_page->setText(QStringLiteral("第 %1 页 / 共 %2 页").arg(m_curPage).arg(m_totalPage));
    m_btn_prev->setEnabled(m_curPage > 1);
    m_btn_next->setEnabled(m_curPage < m_totalPage);

    const int offset = (m_curPage - 1) * m_pageSize;
    QSqlQuery query(db);
    QString sql = "SELECT id, channel_id, image_name, capture_time, image_path "
                  "FROM feature_image WHERE capture_time >= ? AND capture_time < ?";
    if (selectedChannelFilter() > 0) {
        sql += " AND channel_id = ?";
    }
    sql += QString(" ORDER BY capture_time DESC LIMIT %1, %2").arg(offset).arg(m_pageSize);

    query.prepare(sql);
    query.addBindValue(selectedDateStart().toString("yyyy-MM-dd HH:mm:ss"));
    query.addBindValue(selectedDateEnd().toString("yyyy-MM-dd HH:mm:ss"));
    if (selectedChannelFilter() > 0) {
        query.addBindValue(selectedChannelFilter());
    }

    if (!query.exec()) {
        QMessageBox::critical(this, QStringLiteral("查询失败"), query.lastError().text());
        return;
    }

    bool hasResult = false;
    while (query.next()) {
        hasResult = true;
        QListWidgetItem *item = new QListWidgetItem(query.value("image_name").toString());
        item->setData(Qt::UserRole, query.value("image_path").toString());
        item->setData(Qt::UserRole + 1, query.value("id").toInt());
        item->setData(Qt::UserRole + 2, query.value("channel_id").toInt());
        item->setData(Qt::UserRole + 3, query.value("capture_time").toString());
        m_list_record->addItem(item);
    }

    if (!hasResult) {
        showEmptyPlaceholder(QStringLiteral("当前筛选条件下暂无抓拍图片"));
    }
}

int ReviewWidget::getVideoTotalCount() const
{
    QSqlDatabase db = DbConn::getInstence()->getDb();
    if (!db.isOpen()) {
        return 0;
    }

    QSqlQuery query(db);
    QString sql = "SELECT COUNT(*) FROM video WHERE start_time >= ? AND start_time < ?";
    if (selectedChannelFilter() > 0) {
        sql += " AND channel_id = ?";
    }

    query.prepare(sql);
    query.addBindValue(selectedDateStart().toString("yyyy-MM-dd HH:mm:ss"));
    query.addBindValue(selectedDateEnd().toString("yyyy-MM-dd HH:mm:ss"));
    if (selectedChannelFilter() > 0) {
        query.addBindValue(selectedChannelFilter());
    }

    if (!query.exec() || !query.next()) {
        return 0;
    }

    return query.value(0).toInt();
}

int ReviewWidget::getImageTotalCount() const
{
    QSqlDatabase db = DbConn::getInstence()->getDb();
    if (!db.isOpen()) {
        return 0;
    }

    QSqlQuery query(db);
    QString sql = "SELECT COUNT(*) FROM feature_image WHERE capture_time >= ? AND capture_time < ?";
    if (selectedChannelFilter() > 0) {
        sql += " AND channel_id = ?";
    }

    query.prepare(sql);
    query.addBindValue(selectedDateStart().toString("yyyy-MM-dd HH:mm:ss"));
    query.addBindValue(selectedDateEnd().toString("yyyy-MM-dd HH:mm:ss"));
    if (selectedChannelFilter() > 0) {
        query.addBindValue(selectedChannelFilter());
    }

    if (!query.exec() || !query.next()) {
        return 0;
    }

    return query.value(0).toInt();
}

void ReviewWidget::insertFeatureImageRecord(const QString &filePath)
{
    QSqlDatabase db = DbConn::getInstence()->getDb();
    if (!db.isOpen()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("数据库未连接，截图只保存到了目录"));
        return;
    }

    const QFileInfo fileInfo(filePath);
    const QString imageName = fileInfo.fileName();
    const QString captureTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    QSqlQuery query(db);
    query.prepare("INSERT INTO feature_image(image_name, image_path, channel_id, capture_time, create_time) "
                  "VALUES(?, ?, ?, ?, ?)");
    query.addBindValue(imageName);
    query.addBindValue(filePath);
    query.addBindValue(channelid);
    query.addBindValue(captureTime);
    query.addBindValue(captureTime);

    if (query.exec()) {
        return;
    }

    QSqlQuery fallbackQuery(db);
    fallbackQuery.prepare("INSERT INTO feature_image(image_name, image_path, channel_id, capture_time) "
                          "VALUES(?, ?, ?, ?)");
    fallbackQuery.addBindValue(imageName);
    fallbackQuery.addBindValue(filePath);
    fallbackQuery.addBindValue(channelid);
    fallbackQuery.addBindValue(captureTime);

    if (!fallbackQuery.exec()) {
        QMessageBox::warning(this,
                             QStringLiteral("提示"),
                             QStringLiteral("截图已保存，但图片记录写库失败：%1")
                                 .arg(fallbackQuery.lastError().text()));
        return;
    }
}

void ReviewWidget::showEmptyPlaceholder(const QString &text)
{
    QListWidgetItem *item = new QListWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
    m_list_record->addItem(item);
}
