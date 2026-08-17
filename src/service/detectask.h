#ifndef DETECTTASK_H
#define DETECTTASK_H

#include <QImage>
#include <QList>
#include <QMutex>
#include <QObject>
#include <QRect>
#include <QString>

#include "Detect.h"

#include <opencv2/opencv.hpp>

#include <memory>

struct DetectionBox
{
    QRect rect;
    int classId = -1;
    float confidence = 0.0f;
    QString className;
};

using DetectionBoxes = QList<DetectionBox>;
Q_DECLARE_METATYPE(DetectionBox)
Q_DECLARE_METATYPE(DetectionBoxes)

class DetectTask : public QObject
{
    Q_OBJECT

public:
    explicit DetectTask(int channelId, QObject *parent = nullptr);

    void releaseNet();
    void submitFrame(int channelId, qint64 frameId, const QImage &frame);

signals:
    void sendDetectionResult(int channelId, qint64 frameId, const DetectionBoxes &boxes);

public slots:
    void setDetectEnable(bool enable);
    void processLatestFrame();

private:
    bool ensureDetector();
    DetectionBoxes detectObjects(const cv::Mat &cvFrame);

    int m_channelId;
    std::unique_ptr<Detect> m_detector;
    QMutex m_frameMutex;
    QImage m_pendingFrame;
    qint64 m_pendingFrameId = -1;
    bool m_hasPendingFrame = false;
    bool m_processScheduled = false;
    bool m_detectEnable = false;
    bool m_threadLogged = false;
    int m_debugFramesLeft = 5;
};

#endif // DETECTTASK_H
