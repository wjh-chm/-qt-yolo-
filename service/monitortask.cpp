#include "monitortask.h"

#include <QDateTime>
#include <QFileInfo>
#include <QMutexLocker>

#include <algorithm>
#include <cmath>

#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

MonitorTask::MonitorTask(int channelId, QObject *parent)
    : QThread(parent),
      m_channelId(channelId),
      m_sourceType(InputSourceType::Camera),
      m_cameraIndex(0),
      m_segmentDurationSeconds(30),
      m_recordingEnabled(false),
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
    m_recordTask.setRecordRoot(recordRoot);
}

void MonitorTask::setSegmentDurationSeconds(int seconds)
{
    QMutexLocker locker(&m_stateMutex);
    m_segmentDurationSeconds = seconds > 0 ? seconds : 30;
}

void MonitorTask::stop()
{
    m_stopRequested.store(true);
}

void MonitorTask::setRecordingEnabled(bool enabled)
{
    QMutexLocker locker(&m_stateMutex);
    m_recordingEnabled = enabled;
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
        runningMessage = QStringLiteral("摄像头预览中");
        if (!opened) {
            emit statusChanged(m_channelId, QStringLiteral("摄像头打开失败"));
            return;
        }
    } else {
        const QFileInfo fileInfo(videoFilePath);
        if (!fileInfo.exists() || !fileInfo.isFile()) {
            emit statusChanged(m_channelId, QStringLiteral("视频文件不存在"));
            return;
        }

        opened = capture.open(videoFilePath.toLocal8Bit().constData());
        runningMessage = QStringLiteral("演示视频预览中");
        if (!opened) {
            emit statusChanged(m_channelId, QStringLiteral("视频文件打开失败"));
            return;
        }
    }

    double fps = capture.get(cv::CAP_PROP_FPS);
    if (!std::isfinite(fps) || fps < 1.0 || fps > 120.0) {
        fps = 25.0;
    }

    emit statusChanged(m_channelId, runningMessage);
    const int frameDelayMs = std::max(1, static_cast<int>(1000.0 / fps));

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

        bool shouldRecord = false;
        int segmentDurationSeconds = 30;
        {
            QMutexLocker locker(&m_stateMutex);
            shouldRecord = m_recordingEnabled;
            segmentDurationSeconds = m_segmentDurationSeconds;
        }

        if (shouldRecord && !m_recordTask.isRecording()) {
            const bool recordOpened = m_recordTask.start(m_channelId, fps, frame.size());
            emit recordingChanged(m_channelId,
                                  recordOpened,
                                  m_recordTask.currentVideo().filePath(),
                                  m_recordTask.currentVideo().startTime(),
                                  m_recordTask.currentVideo().endTime());
            if (!recordOpened) {
                emit statusChanged(m_channelId, QStringLiteral("录像文件打开失败"));
            }
        } else if (shouldRecord && m_recordTask.isRecording()
                   && m_recordTask.currentVideo().startTime().isValid()
                   && m_recordTask.currentVideo().startTime().secsTo(QDateTime::currentDateTime())
                          >= segmentDurationSeconds) {
            const Video finishedVideo = m_recordTask.stop();
            emit recordingChanged(m_channelId,
                                  false,
                                  finishedVideo.filePath(),
                                  finishedVideo.startTime(),
                                  finishedVideo.endTime());

            const bool recordOpened = m_recordTask.start(m_channelId, fps, frame.size());
            emit recordingChanged(m_channelId,
                                  recordOpened,
                                  m_recordTask.currentVideo().filePath(),
                                  m_recordTask.currentVideo().startTime(),
                                  m_recordTask.currentVideo().endTime());
            if (!recordOpened) {
                emit statusChanged(m_channelId, QStringLiteral("录像文件打开失败"));
            }
        } else if (!shouldRecord && m_recordTask.isRecording()) {
            const Video video = m_recordTask.stop();
            emit recordingChanged(m_channelId, false, video.filePath(), video.startTime(), video.endTime());
        }

        m_recordTask.write(frame);
        emit frameReady(m_channelId, matToImage(frame));
        msleep(frameDelayMs);
    }

    if (m_recordTask.isRecording()) {
        const Video video = m_recordTask.stop();
        emit recordingChanged(m_channelId, false, video.filePath(), video.startTime(), video.endTime());
    }

    capture.release();
    emit statusChanged(m_channelId, QStringLiteral("已停止"));
}

QImage MonitorTask::matToImage(const cv::Mat &frame) const
{
    cv::Mat rgbFrame;
    if (frame.channels() == 3) {
        cv::cvtColor(frame, rgbFrame, cv::COLOR_BGR2RGB);
    } else if (frame.channels() == 4) {
        cv::cvtColor(frame, rgbFrame, cv::COLOR_BGRA2RGBA);
    } else {
        cv::cvtColor(frame, rgbFrame, cv::COLOR_GRAY2RGB);
    }

    return QImage(rgbFrame.data,
                  rgbFrame.cols,
                  rgbFrame.rows,
                  static_cast<int>(rgbFrame.step),
                  rgbFrame.channels() == 4 ? QImage::Format_RGBA8888 : QImage::Format_RGB888)
        .copy();
}
