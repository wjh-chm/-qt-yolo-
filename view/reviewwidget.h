#ifndef REVIEWWIDGET_H
#define REVIEWWIDGET_H

#include "../util/dbconn.h"

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
    explicit ReviewWidget(QWidget *parent = nullptr);

    void loadPageData(int page);
    int getTotalCount();

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
    QLabel *m_image_label;

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
};

#endif
