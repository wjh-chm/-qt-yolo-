#ifndef REVIEWWIDGET_H
#define REVIEWWIDGET_H
#include <QJsonArray>
#include <QJsonObject>
#include "detailwidget.h"

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

class ClientApi;

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
    void refreshData();
    void setClientApi(ClientApi *api);

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
    bool m_waitingVideoList = false;//状态变量防止多个回放界面都收到同一个响应
    bool m_waitingImageList = false; //等待标志，等待连接成功
    bool m_waitingInsertImage = false;
    bool m_waitingDeleteImages = false;
    bool m_waitingRelatedVideo = false;
    qint64 m_videoListRequestId = 0;
    qint64 m_imageListRequestId = 0;
    qint64 m_insertImageRequestId = 0;
    qint64 m_deleteImagesRequestId = 0;
    qint64 m_relatedVideoRequestId = 0;
    QDateTime m_pendingRelatedCaptureTime;
    QList<DetailImageItem> m_pendingDeleteImages;
    ClientApi *m_clientApi = nullptr;

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

private slots:
    void itemselected(QListWidgetItem *item);
    void onsavedClicked();
    void onVideoListFinished(qint64 requestId,
                             bool success,
                             const QJsonArray &list,
                             int totalCount,
                             const QString &message);

    void onImageListFinished(qint64 requestId,
                             bool success,
                             const QJsonArray &list,
                             int totalCount,
                             const QString &message);

    void onRelatedVideoFinished(qint64 requestId,
                                bool success,
                                const QJsonObject &record,
                                const QString &message);

    void onInsertImageFinished(qint64 requestId,
                               bool success,
                               int imageId,
                               const QString &message);

    void onDeleteImagesFinished(qint64 requestId,
                                bool success,
                                const QString &message);
};

#endif
