#ifndef RECORDTASK_H
#define RECORDTASK_H

#include "model/video.h"

#include <QString>

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>

// 轻量级录像辅助类，负责管理当前录制会话对应的 OpenCV 写入器。
class RecordTask
{
public:
    RecordTask();
    ~RecordTask();

    void setRecordRoot(const QString &recordRoot);

    // 按通道号和画面格式打开一个新的录像文件。
    bool start(int channelId, double fps, const cv::Size &frameSize);

    // 录制开启时写入一帧图像。
    void write(const cv::Mat &frame);

    // 关闭写入器，并返回这次录制对应的视频元数据。
    Video stop();
    bool isRecording() const;
    Video currentVideo() const;

private:
    // 生成带时间戳的文件路径，避免多次录制时重名。
    QString nextRecordFilePath(int channelId) const;

    QString m_recordRoot;     // 录像输出目录，生成新文件时会以它为根路径。
    cv::VideoWriter m_writer; // OpenCV 写文件对象，打开后表示当前正在录制。
    Video m_currentVideo;     // 当前录制会话对应的录像元数据，停止录制时返回给上层。
};

#endif // RECORDTASK_H
