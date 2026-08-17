#ifndef MONITORWIDGET_H
#define MONITORWIDGET_H

#include "model/exceptionmodel.h"
#include "model/imagemodel.h"
#include "model/videomodel.h"
#include "service/detectask.h"
#include "service/monitortask.h"

#include <QCheckBox>
#include <QDateTime>
#include <QFile>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPainter>
#include <QPushButton>
#include <QRect>
#include <QStackedLayout>
#include <QThread>
#include <QVBoxLayout>
#include <QWidget>

#include <array>

class Monitorwidget : public QWidget
{
    Q_OBJECT

public:
    explicit Monitorwidget(QWidget *parent = nullptr);
    ~Monitorwidget() override;

    void setLoginUser(const QString &username);
    bool isLoggedIn() const;
    void init_leftwidget();
    void init_rightwidget();
    void init_monitorqss();
    void init_connect();

private:
    struct ChannelConfig {
        int channelId;
        QString displayName;
        bool useCamera;
        int cameraIndex;
        QString videoFilePath;
    };

    void initChannelConfigs();
    void bindDeviceList();
    void startChannels(int channelCount);
    void startChannel(int channelId);
    void stopAllChannels(bool resetRecordingState = true);
    void stopDetectChannel(int channelId);
    void showPlaceholder(int channelId, const QString &message);
    void showFrame(QLabel *label, const QImage &frame);
    QImage composeDisplayFrame(int channelId) const;
    bool isDetectionResultExpired(int channelId, qint64 resultFrameId) const;
    bool isDetectionInputEnabled(int channelId) const;
    void refreshDetectionTaskEnables();
    void clearDetectionBoxes(int channelId);
    bool hasRunningTask() const;
    void saveVideoRecord(int channelId,
                         const QString &filePath,
                         const QDateTime &startTime,
                         const QDateTime &endTime);
    void applyLoadedSettings();
    void applyRecordingState();
    void ensurePreviewRunning();
    QString channelName(int channelId) const;

    QWidget *left_widget;
    QWidget *right_widget;
    QListWidget *device_list;
    QPushButton *btn_channel1;
    QPushButton *btn_channel4;
    QCheckBox *m_check_motionDetect;
    QLabel *lab_video;
    QLabel *lab_videos[4];
    QLabel *lab_status;
    QStackedLayout *video_layout;
    std::array<MonitorTask *, 4> m_tasks;
    std::array<QThread *, 4> m_detectThreads;
    std::array<DetectTask *, 4> m_detectTasks;
    std::array<QImage, 4> m_latestFrames;
    std::array<qint64, 4> m_latestFrameIds;
    std::array<DetectionBoxes, 4> m_latestDetectionBoxes;
    std::array<ChannelConfig, 4> m_channelConfigs;
    QString m_recordRoot;
    int m_segmentDurationSeconds;
    QString m_loginUser;
    VideoModel m_videoModel;
    ImageModel m_imageModel;
    ExceptionModel m_exceptionModel;
    int m_selectedChannel;
    bool m_recording;
    bool m_detecting;

public slots:
    void swtich_to_channel1();
    void swtich_to_channel4();
    void start_single_channel();
    void start_four_channel();
    void select_channel(QListWidgetItem *item);
    void on_frame_ready(int channelId, qint64 frameId, const QImage &frame);
    void on_detect_result(int channelId, qint64 frameId, const DetectionBoxes &boxes);
    void on_status_changed(int channelId, const QString &message);
    void on_recording_changed(int channelId,
                              bool recording,
                              const QString &filePath,
                              const QDateTime &startTime,
                              const QDateTime &endTime);
    void on_anomaly_session_ready(const Exception &exception,
                                  const Video &video,
                                  const ImageList &images);
    void reloadStorageSettings();
    void slotDetectToggle(Qt::CheckState state);
};

#endif // MONITORWIDGET_H
