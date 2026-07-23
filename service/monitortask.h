#ifndef MONITORTASK_H
#define MONITORTASK_H

#include <QImage>
#include <QMutex>
#include <QString>
#include <QThread>

#include <atomic>

#include <opencv2/core/mat.hpp>

#include "recordtask.h"

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
    void stop();

public slots:
    void setRecordingEnabled(bool enabled);

signals:
    void frameReady(int channelId, const QImage &frame);
    void statusChanged(int channelId, const QString &message);
    void recordingChanged(int channelId,
                          bool recording,
                          const QString &filePath,
                          const QDateTime &startTime,
                          const QDateTime &endTime);

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
    bool m_recordingEnabled;
    std::atomic_bool m_stopRequested;
    RecordTask m_recordTask;
};

#endif // MONITORTASK_H
