#include "recordtask.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>

RecordTask::RecordTask()
    : m_recordRoot(QCoreApplication::applicationDirPath() + "/records")
{
}

RecordTask::~RecordTask()
{
    if (m_writer.isOpened()) {
        m_writer.release();
    }
}

void RecordTask::setRecordRoot(const QString &recordRoot)
{
    m_recordRoot = recordRoot;
}

bool RecordTask::start(int channelId, double fps, const cv::Size &frameSize)
{
    if (m_writer.isOpened()) {
        return true;
    }

    const QString filePath = nextRecordFilePath(channelId);
    QDir().mkpath(QFileInfo(filePath).absolutePath());

    const int fourcc = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
    const bool opened = m_writer.open(filePath.toLocal8Bit().constData(),
                                      fourcc,
                                      fps,
                                      frameSize,
                                      true);
    if (!opened) {
        m_currentVideo = Video();
        return false;
    }

    m_currentVideo = Video(channelId + 1,
                           QFileInfo(filePath).fileName(),
                           filePath,
                           QDateTime::currentDateTime());
    return true;
}

void RecordTask::write(const cv::Mat &frame)
{
    if (m_writer.isOpened() && !frame.empty()) {
        m_writer.write(frame);
    }
}

Video RecordTask::stop()
{
    if (m_writer.isOpened()) {
        m_writer.release();
    }

    Video video;
    if (m_currentVideo.isValid()) {
        video = Video(m_currentVideo.channelId(),
                      m_currentVideo.videoName(),
                      m_currentVideo.filePath(),
                      m_currentVideo.startTime(),
                      QDateTime::currentDateTime());
    }

    m_currentVideo = Video();
    return video;
}

bool RecordTask::isRecording() const
{
    return m_writer.isOpened();
}

Video RecordTask::currentVideo() const
{
    return m_currentVideo;
}

QString RecordTask::nextRecordFilePath(int channelId) const
{
    const QString stamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz");
    return QDir(m_recordRoot).filePath(QString("channel_%1_%2.avi").arg(channelId + 1).arg(stamp));
}
