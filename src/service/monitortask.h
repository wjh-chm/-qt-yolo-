#ifndef MONITORTASK_H
#define MONITORTASK_H

#include "model/exception.h"
#include "model/image.h"
#include "recordtask.h"

#include <QDateTime>
#include <QImage>
#include <QMutex>
#include <QString>
#include <QThread>

#include <atomic>

#include <opencv2/core/mat.hpp>

class MonitorTask : public QThread
{
    Q_OBJECT

public:
    enum class InputSourceType {
        Camera,
        VideoFile
    };

    explicit MonitorTask(int channelId, QObject *parent = nullptr);
    ~MonitorTask() override;

    void setCameraSource(int cameraIndex);
    void setVideoFileSource(const QString &filePath);
    void setRecordRoot(const QString &recordRoot);
    void setSegmentDurationSeconds(int seconds);
    void setAnomalyDurationSeconds(int seconds);
    void setAnomalyVideoRoot(const QString &path);
    void setAnomalyImageRoot(const QString &path);
    void notifyAnomalyDetected();
    void stop();

public slots:
    void setRecordingEnabled(bool enabled);

signals:
    void frameReady(int channelId, qint64 frameId, const QImage &frame);
    void statusChanged(int channelId, const QString &message);
    void recordingChanged(int channelId,
                          bool recording,
                          const QString &filePath,
                          const QDateTime &startTime,
                          const QDateTime &endTime);
    void anomalySessionReady(const Exception &exception,
                             const Video &video,
                             const ImageList &images);

protected:
    void run() override;

private:
    QImage matToImage(const cv::Mat &frame) const;

    int m_channelId;
    mutable QMutex m_stateMutex;
    InputSourceType m_sourceType;
    int m_cameraIndex;
    QString m_videoFilePath;
    QString m_recordRoot;
    int m_segmentDurationSeconds;
    bool m_normalRecordingEnabled;
    QString m_anomalyVideoRoot;
    QString m_anomalyImageRoot;
    int m_anomalyDurationSeconds;
    bool m_anomalyPending;
    bool m_anomalyActive;
    std::atomic_bool m_stopRequested;
    RecordTask m_normalRecordTask;
    RecordTask m_anomalyRecordTask;
};

#endif // MONITORTASK_H
