#ifndef REVIEWWIDGET_H
#define REVIEWWIDGET_H

#include "detailwidget.h"

#include "model/imagemodel.h"
#include "model/videomodel.h"
#include <QComboBox>
#include <QCoreApplication>
#include <QDateEdit>
#include <QFileDialog>
#include <QImage>
#include <QLabel>
#include <QListWidget>
#include <QMediaPlayer>
#include <QPushButton>
#include <QSlider>
#include <QStackedWidget>
#include <QString>
#include <QVideoWidget>
#include <QWidget>

class ReviewWidget : public QWidget
{
    Q_OBJECT

public:
    enum class ReviewScope {
        NormalOnly,
        AnomalyOnly
    };

    explicit ReviewWidget(ReviewScope scope = ReviewScope::NormalOnly, QWidget *parent = nullptr);

    void loadPageData(int page);
    int getTotalCount();
    void refreshData();

private:
    enum class RecordMode {
        Video,
        Image
    };

    void initUI();
    void previewMedia(const QString &path);
    void play();
    QString formatTime(qint64 ms) const;
    bool isImageFile(const QString &suffix) const;
    bool isVideoFile(const QString &suffix) const;
    void resetPlaybackUi();
    int selectedChannelFilter() const;
    QDateTime selectedDateStart() const;
    QDateTime selectedDateEnd() const;
    void resetCurrentPreviewState();
    QDate resolveInitialFilterDate() const;
    void switchRecordMode(RecordMode mode);
    void loadVideoPageData(int page);
    void loadImagePageData(int page);
    int getVideoTotalCount() const;
    int getImageTotalCount() const;
    void insertFeatureImageRecord(const QString &filePath);
    void showEmptyPlaceholder(const QString &text);
    QString scopeTitle() const;
    bool isAnomalyScope() const;
    void openRelatedVideoForImage(int channelId, const QDateTime &captureTime);
    void openVideoAtTimestamp(const QString &videoPath,
                              const QDateTime &startTime,
                              const QDateTime &captureTime);
    void deleteImages(const QList<DetailImageItem> &images);
    void exportImages(const QList<DetailImageItem> &images);
    QString uniqueExportPath(const QString &targetDir, const QString &fileName) const;

    ReviewScope m_scope;
    RecordMode m_recordMode;
    int m_pageSize;
    int m_curPage;
    int m_totalPage;
    int channelid;

    QDateEdit *m_edit_date;
    QComboBox *m_box_channel;
    QListWidget *m_list_record;

    QPushButton *m_btn_prev;
    QPushButton *m_btn_next;
    QLabel *m_lab_page;

    QWidget *m_right_panel;
    QPushButton *m_btn_video_preview;
    QPushButton *m_btn_image_preview;

    QStackedWidget *m_preview_stack;
    QWidget *m_video_page;
    QWidget *m_image_page;
    QVideoWidget *m_video_widget;
    DetailWidget *m_detail_widget;

    QMediaPlayer *m_player;

    QPushButton *btn_save;
    QPushButton *btn_play;
    QComboBox *box_speed;
    QSlider *m_slider_progress;
    QLabel *m_lab_current_time;
    QLabel *m_lab_total_time;

    QString currentPath;
    QString m_loadedPath;
    QImage m_lastVideoFrame;
    ImageModel m_imageModel;
    VideoModel m_videoModel;

private slots:
    void itemselected(QListWidgetItem *item);
    void onsavedClicked();
};

#endif
