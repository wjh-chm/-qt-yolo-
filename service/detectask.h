#ifndef DETECTTASK_H
#define DETECTTASK_H

#include <QImage>
#include <QObject>

#include <opencv2/dnn.hpp>
#include <opencv2/opencv.hpp>

class DetectTask : public QObject
{
    Q_OBJECT

public:
    explicit DetectTask(int channelId, QObject *parent = nullptr);

    void releaseNet();

signals:
    void sendDrawFrame(int channelId, const QImage &outFrame);

public slots:
    void recvFrame(int channelId, const QImage &frame);
    void setDetectEnable(bool enable);

private:
    int m_channelId;
    cv::dnn::Net m_yoloNet;
    bool m_detectEnable = false;
    bool m_threadLogged = false;
    int m_debugFramesLeft = 5;
    const float m_confThresh = 0.25f;
    const int m_inputSize = 320;
};

#endif // DETECTTASK_H
