#include "monitortask.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMutexLocker>

#include <algorithm>
#include <cmath>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

namespace {
bool shouldCaptureAnomalyImage(int frameIndex)
{
    return frameIndex == 0 || frameIndex == 10 || frameIndex == 20;
}

QString buildAnomalyImagePath(const QString &root,
                              int channelId,
                              const QDateTime &captureTime,
                              int frameIndex)
{
    return QDir(root).filePath(
        QString("异常_ch%1_%2_f%3.jpg")
            .arg(channelId + 1)
            .arg(captureTime.toString("yyyyMMdd_hhmmss_zzz"))
            .arg(frameIndex));
}
} // namespace

MonitorTask::MonitorTask(int channelId, QObject *parent)
    : QThread(parent),
      m_channelId(channelId),
      m_sourceType(InputSourceType::Camera),
      m_cameraIndex(0),
      m_recordRoot(QDir(QCoreApplication::applicationDirPath()).filePath("records")),
      m_segmentDurationSeconds(30),
      m_normalRecordingEnabled(false),
      m_anomalyVideoRoot(QDir(QCoreApplication::applicationDirPath()).filePath("records/anomaly")),
      m_anomalyImageRoot(QDir(QCoreApplication::applicationDirPath()).filePath("feature/anomaly")),
      m_anomalyDurationSeconds(10),
      m_anomalyPending(false),
      m_anomalyActive(false),
      m_stopRequested(false)
{
}

MonitorTask::~MonitorTask()
{
    stop();
    wait(1500);
}

void MonitorTask::setCameraSource(int cameraIndex)
{
    QMutexLocker locker(&m_stateMutex);
    m_sourceType = InputSourceType::Camera;
    m_cameraIndex = cameraIndex;
    m_videoFilePath.clear();
}

void MonitorTask::setVideoFileSource(const QString &filePath)
{
    QMutexLocker locker(&m_stateMutex);
    m_sourceType = InputSourceType::VideoFile;
    m_videoFilePath = filePath;
}

void MonitorTask::setRecordRoot(const QString &recordRoot)
{
    QMutexLocker locker(&m_stateMutex);
    m_recordRoot = recordRoot;
}

void MonitorTask::setSegmentDurationSeconds(int seconds)
{
    QMutexLocker locker(&m_stateMutex);
    m_segmentDurationSeconds = seconds > 0 ? seconds : 30;
}

void MonitorTask::setAnomalyDurationSeconds(int seconds)
{
    QMutexLocker locker(&m_stateMutex);
    m_anomalyDurationSeconds = seconds > 0 ? seconds : 10;
}

void MonitorTask::setAnomalyVideoRoot(const QString &path)
{
    QMutexLocker locker(&m_stateMutex);
    m_anomalyVideoRoot = path;
}

void MonitorTask::setAnomalyImageRoot(const QString &path)
{
    QMutexLocker locker(&m_stateMutex);
    m_anomalyImageRoot = path;
}

void MonitorTask::notifyAnomalyDetected()
{
    QMutexLocker locker(&m_stateMutex);
    if (m_anomalyActive || m_anomalyPending) {
        return;
    }
    m_anomalyPending = true;
    qDebug() << "[anomaly] detected on channel" << (m_channelId + 1);
}

void MonitorTask::stop()
{
    m_stopRequested.store(true);
}

void MonitorTask::setRecordingEnabled(bool enabled)
{
    QMutexLocker locker(&m_stateMutex);
    m_normalRecordingEnabled = enabled;
}

void MonitorTask::run()
{
    m_stopRequested.store(false);

    InputSourceType sourceType = InputSourceType::Camera;
    int cameraIndex = 0;
    QString videoFilePath;
    {
        QMutexLocker locker(&m_stateMutex);
        sourceType = m_sourceType;
        cameraIndex = m_cameraIndex;
        videoFilePath = m_videoFilePath;
    }

    cv::VideoCapture capture;
    bool opened = false;
    QString runningMessage;

    if (sourceType == InputSourceType::Camera) {
        opened = capture.open(cameraIndex, cv::CAP_DSHOW) || capture.open(cameraIndex);
        runningMessage = QStringLiteral("camera preview running");
        if (!opened) {
            emit statusChanged(m_channelId, QStringLiteral("camera open failed"));
            return;
        }
    } else {
        const QFileInfo fileInfo(videoFilePath);
        if (!fileInfo.exists() || !fileInfo.isFile()) {
            emit statusChanged(m_channelId, QStringLiteral("video file missing"));
            return;
        }

        opened = capture.open(videoFilePath.toLocal8Bit().constData());
        runningMessage = QStringLiteral("demo video preview running");
        if (!opened) {
            emit statusChanged(m_channelId, QStringLiteral("video file open failed"));
            return;
        }
    }

    double fps = capture.get(cv::CAP_PROP_FPS);
    if (!std::isfinite(fps) || fps < 1.0 || fps > 120.0) {
        fps = 25.0;
    }

    emit statusChanged(m_channelId, runningMessage);
    const int frameDelayMs = std::max(1, static_cast<int>(1000.0 / fps));

    QDateTime anomalyStartTime;
    QDateTime anomalyEndTime;
    ImageList anomalyImages;
    int anomalyFrameIndex = 0;
    qint64 frameId = 0;

    while (!m_stopRequested.load()) {
        cv::Mat frame;
        capture >> frame;

        if (frame.empty()) {
            if (sourceType == InputSourceType::VideoFile) {
                capture.set(cv::CAP_PROP_POS_FRAMES, 0);
                capture >> frame;
            }

            if (frame.empty()) {
                msleep(20);
                continue;
            }
        }

        bool normalRecordingEnabled = false;
        int segmentDurationSeconds = 30;
        int anomalyDurationSeconds = 10;
        QString normalRecordRoot;
        QString anomalyVideoRoot;
        QString anomalyImageRoot;
        bool anomalyPending = false;
        bool anomalyActive = false;
        {
            QMutexLocker locker(&m_stateMutex);
            normalRecordingEnabled = m_normalRecordingEnabled;
            segmentDurationSeconds = m_segmentDurationSeconds;
            anomalyDurationSeconds = m_anomalyDurationSeconds;
            normalRecordRoot = m_recordRoot;
            anomalyVideoRoot = m_anomalyVideoRoot;
            anomalyImageRoot = m_anomalyImageRoot;
            anomalyPending = m_anomalyPending;
            anomalyActive = m_anomalyActive;
        }

        m_normalRecordTask.setRecordRoot(normalRecordRoot);
        m_anomalyRecordTask.setRecordRoot(anomalyVideoRoot);

        if (anomalyPending && !anomalyActive) {
            {
                QMutexLocker locker(&m_stateMutex);
                m_anomalyPending = false;
            }

            if (normalRecordingEnabled) {
                if (m_normalRecordTask.isRecording()) {
                    const Video finishedNormalVideo = m_normalRecordTask.stop();
                    emit recordingChanged(m_channelId,
                                          false,
                                          finishedNormalVideo.filePath(),
                                          finishedNormalVideo.startTime(),
                                          finishedNormalVideo.endTime());
                }

                const bool anomalyOpened = m_anomalyRecordTask.start(m_channelId, fps, frame.size());
                if (anomalyOpened) {
                    anomalyStartTime = QDateTime::currentDateTime();
                    anomalyEndTime = anomalyStartTime.addSecs(anomalyDurationSeconds > 0
                                                                  ? anomalyDurationSeconds
                                                                  : 10);
                    anomalyImages.clear();
                    anomalyFrameIndex = 0;

                    {
                        QMutexLocker locker(&m_stateMutex);
                        m_anomalyActive = true;
                    }
                    anomalyActive = true;
                    qDebug() << "[anomaly] start recording channel" << (m_channelId + 1)
                             << "video path:" << m_anomalyRecordTask.currentVideo().filePath();
                    emit statusChanged(m_channelId, QStringLiteral("anomaly recording started"));
                } else {
                    emit statusChanged(m_channelId, QStringLiteral("anomaly video open failed"));
                }
            }
        }

        if (anomalyActive) {
            m_anomalyRecordTask.write(frame);

            if (shouldCaptureAnomalyImage(anomalyFrameIndex)) {
                const QDateTime captureTime = QDateTime::currentDateTime();
                const QString imagePath =
                    buildAnomalyImagePath(anomalyImageRoot, m_channelId, captureTime, anomalyFrameIndex);
                QDir().mkpath(QFileInfo(imagePath).absolutePath());
                if (cv::imwrite(imagePath.toLocal8Bit().constData(), frame)) {
                    qDebug() << "[anomaly] save image channel" << (m_channelId + 1)
                             << "frame index:" << anomalyFrameIndex
                             << "path:" << imagePath;
                    anomalyImages.append(Image(m_channelId + 1,
                                               QFileInfo(imagePath).fileName(),
                                               imagePath,
                                               captureTime,
                                               captureTime));
                }
            }

            ++anomalyFrameIndex;

            if (QDateTime::currentDateTime() >= anomalyEndTime) {
                const Video anomalyVideo = m_anomalyRecordTask.stop();
                if (anomalyVideo.isValid()) {
                    qDebug() << "[anomaly] finish recording channel" << (m_channelId + 1)
                             << "video path:" << anomalyVideo.filePath();
                    const Exception exceptionRecord(m_channelId + 1,
                                                    anomalyStartTime,
                                                    anomalyVideo.filePath());
                    emit anomalySessionReady(exceptionRecord, anomalyVideo, anomalyImages);
                }

                anomalyImages.clear();
                anomalyFrameIndex = 0;
                anomalyStartTime = QDateTime();
                anomalyEndTime = QDateTime();
                {
                    QMutexLocker locker(&m_stateMutex);
                    m_anomalyActive = false;
                    m_anomalyPending = false;
                }
                emit statusChanged(m_channelId, QStringLiteral("anomaly recording finished"));
            }
        } else {
            if (normalRecordingEnabled && !m_normalRecordTask.isRecording()) {
                const bool recordOpened = m_normalRecordTask.start(m_channelId, fps, frame.size());
                emit recordingChanged(m_channelId,
                                      recordOpened,
                                      m_normalRecordTask.currentVideo().filePath(),
                                      m_normalRecordTask.currentVideo().startTime(),
                                      m_normalRecordTask.currentVideo().endTime());
                if (!recordOpened) {
                    emit statusChanged(m_channelId, QStringLiteral("record file open failed"));
                }
            } else if (normalRecordingEnabled && m_normalRecordTask.isRecording()
                       && m_normalRecordTask.currentVideo().startTime().isValid()
                       && m_normalRecordTask.currentVideo().startTime().secsTo(QDateTime::currentDateTime())
                              >= segmentDurationSeconds) {
                const Video finishedVideo = m_normalRecordTask.stop();
                emit recordingChanged(m_channelId,
                                      false,
                                      finishedVideo.filePath(),
                                      finishedVideo.startTime(),
                                      finishedVideo.endTime());

                const bool recordOpened = m_normalRecordTask.start(m_channelId, fps, frame.size());
                emit recordingChanged(m_channelId,
                                      recordOpened,
                                      m_normalRecordTask.currentVideo().filePath(),
                                      m_normalRecordTask.currentVideo().startTime(),
                                      m_normalRecordTask.currentVideo().endTime());
                if (!recordOpened) {
                    emit statusChanged(m_channelId, QStringLiteral("record file open failed"));
                }
            } else if (!normalRecordingEnabled && m_normalRecordTask.isRecording()) {
                const Video video = m_normalRecordTask.stop();
                emit recordingChanged(m_channelId, false, video.filePath(), video.startTime(), video.endTime());
            }

            m_normalRecordTask.write(frame);
        }

        emit frameReady(m_channelId, frameId++, matToImage(frame));
        msleep(frameDelayMs);
    }

    if (m_anomalyRecordTask.isRecording()) {
        const Video anomalyVideo = m_anomalyRecordTask.stop();
        if (anomalyVideo.isValid()) {
            qDebug() << "[anomaly] thread stopping, flush channel" << (m_channelId + 1)
                     << "video path:" << anomalyVideo.filePath();
            const Exception exceptionRecord(m_channelId + 1,
                                            anomalyStartTime.isValid() ? anomalyStartTime
                                                                       : QDateTime::currentDateTime(),
                                            anomalyVideo.filePath());
            emit anomalySessionReady(exceptionRecord, anomalyVideo, anomalyImages);
        }
    }

    {
        QMutexLocker locker(&m_stateMutex);
        m_anomalyActive = false;
        m_anomalyPending = false;
    }

    if (m_normalRecordTask.isRecording()) {
        const Video video = m_normalRecordTask.stop();
        emit recordingChanged(m_channelId, false, video.filePath(), video.startTime(), video.endTime());
    }

    capture.release();
    emit statusChanged(m_channelId, QStringLiteral("stopped"));
}

QImage MonitorTask::matToImage(const cv::Mat &frame) const
{
    if (frame.channels() == 3) {
        return QImage(frame.data,
                      frame.cols,
                      frame.rows,
                      static_cast<int>(frame.step),
                      QImage::Format_BGR888)
            .copy();
    }

    if (frame.channels() == 4) {
        return QImage(frame.data,
                      frame.cols,
                      frame.rows,
                      static_cast<int>(frame.step),
                      QImage::Format_ARGB32)
            .copy();
    }

    if (frame.channels() == 1) {
        return QImage(frame.data,
                      frame.cols,
                      frame.rows,
                      static_cast<int>(frame.step),
                      QImage::Format_Grayscale8)
            .copy();
    }

    cv::Mat bgrFrame;
    cv::cvtColor(frame, bgrFrame, cv::COLOR_BGR2RGB);
    return QImage(bgrFrame.data,
                  bgrFrame.cols,
                  bgrFrame.rows,
                  static_cast<int>(bgrFrame.step),
                  QImage::Format_RGB888)
        .copy();
}
