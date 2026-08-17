#include "detectask.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QMutexLocker>
#include <QStringList>
#include <QThread>

#include <exception>

namespace
{
bool isTargetClass(int classId)
{
    return classId == 0 || classId == 2; // COCO: 0 = person, 2 = car
}

QString cocoClassName(int classId)
{
    static const QStringList names = {
        QStringLiteral("person"), QStringLiteral("bicycle"), QStringLiteral("car"),
        QStringLiteral("motorcycle"), QStringLiteral("airplane"), QStringLiteral("bus"),
        QStringLiteral("train"), QStringLiteral("truck"), QStringLiteral("boat"),
        QStringLiteral("traffic light"), QStringLiteral("fire hydrant"), QStringLiteral("stop sign"),
        QStringLiteral("parking meter"), QStringLiteral("bench"), QStringLiteral("bird"),
        QStringLiteral("cat"), QStringLiteral("dog"), QStringLiteral("horse"),
        QStringLiteral("sheep"), QStringLiteral("cow"), QStringLiteral("elephant"),
        QStringLiteral("bear"), QStringLiteral("zebra"), QStringLiteral("giraffe"),
        QStringLiteral("backpack"), QStringLiteral("umbrella"), QStringLiteral("handbag"),
        QStringLiteral("tie"), QStringLiteral("suitcase"), QStringLiteral("frisbee"),
        QStringLiteral("skis"), QStringLiteral("snowboard"), QStringLiteral("sports ball"),
        QStringLiteral("kite"), QStringLiteral("baseball bat"), QStringLiteral("baseball glove"),
        QStringLiteral("skateboard"), QStringLiteral("surfboard"), QStringLiteral("tennis racket"),
        QStringLiteral("bottle"), QStringLiteral("wine glass"), QStringLiteral("cup"),
        QStringLiteral("fork"), QStringLiteral("knife"), QStringLiteral("spoon"),
        QStringLiteral("bowl"), QStringLiteral("banana"), QStringLiteral("apple"),
        QStringLiteral("sandwich"), QStringLiteral("orange"), QStringLiteral("broccoli"),
        QStringLiteral("carrot"), QStringLiteral("hot dog"), QStringLiteral("pizza"),
        QStringLiteral("donut"), QStringLiteral("cake"), QStringLiteral("chair"),
        QStringLiteral("couch"), QStringLiteral("potted plant"), QStringLiteral("bed"),
        QStringLiteral("dining table"), QStringLiteral("toilet"), QStringLiteral("tv"),
        QStringLiteral("laptop"), QStringLiteral("mouse"), QStringLiteral("remote"),
        QStringLiteral("keyboard"), QStringLiteral("cell phone"), QStringLiteral("microwave"),
        QStringLiteral("oven"), QStringLiteral("toaster"), QStringLiteral("sink"),
        QStringLiteral("refrigerator"), QStringLiteral("book"), QStringLiteral("clock"),
        QStringLiteral("vase"), QStringLiteral("scissors"), QStringLiteral("teddy bear"),
        QStringLiteral("hair drier"), QStringLiteral("toothbrush")
    };

    if (classId >= 0 && classId < names.size()) {
        return names.at(classId);
    }
    return QStringLiteral("class_%1").arg(classId);
}

cv::Mat qImageToCvMat(const QImage &image)
{
    if (image.isNull()) {
        return cv::Mat();
    }

    if (image.format() == QImage::Format_BGR888) {
        return cv::Mat(image.height(),
                       image.width(),
                       CV_8UC3,
                       const_cast<uchar *>(image.constBits()),
                       image.bytesPerLine())
            .clone();
    }

    if (image.format() == QImage::Format_Grayscale8) {
        cv::Mat gray(image.height(),
                     image.width(),
                     CV_8UC1,
                     const_cast<uchar *>(image.constBits()),
                     image.bytesPerLine());
        cv::Mat bgr;
        cv::cvtColor(gray, bgr, cv::COLOR_GRAY2BGR);
        return bgr;
    }

    QImage converted = image.convertToFormat(QImage::Format_BGR888);
    return cv::Mat(converted.height(),
                   converted.width(),
                   CV_8UC3,
                   const_cast<uchar *>(converted.bits()),
                   converted.bytesPerLine())
        .clone();
}

QRect boundedRect(const QRect &rect, const cv::Size &frameSize)
{
    return rect.normalized().intersected(QRect(0, 0, frameSize.width, frameSize.height));
}
}

DetectTask::DetectTask(int channelId, QObject *parent)
    : QObject(parent),
      m_channelId(channelId)
{
}

bool DetectTask::ensureDetector()
{
    if (m_detector != nullptr) {
        return true;
    }

    const QString modelPath =
        QDir(QCoreApplication::applicationDirPath()).filePath("tool/data/model/yolo11n.onnx");

    try {
        m_detector = std::make_unique<Detect>(modelPath.toStdString());
        m_detector->model_input_size = cv::Size(640, 640);
        m_detector->setThresholds(0.7f, 0.8f);
        qDebug() << "DetectTask: YOLOv11 model loaded:" << modelPath
                 << "channel:" << m_channelId;
        return true;
    } catch (const cv::Exception &e) {
        qDebug() << "DetectTask: YOLOv11 model load failed:" << modelPath
                 << QString::fromLocal8Bit(e.what());
    } catch (const std::exception &e) {
        qDebug() << "DetectTask: YOLOv11 model load failed:" << modelPath
                 << QString::fromLocal8Bit(e.what());
    }

    return false;
}

void DetectTask::submitFrame(int channelId, qint64 frameId, const QImage &frame)
{
    if (channelId != m_channelId || frame.isNull()) {
        return;
    }

    bool shouldSchedule = false;
    {
        QMutexLocker locker(&m_frameMutex);
        if (!m_detectEnable) {
            return;
        }

        m_pendingFrame = frame.copy();
        m_pendingFrameId = frameId;
        m_hasPendingFrame = true;
        if (!m_processScheduled) {
            m_processScheduled = true;
            shouldSchedule = true;
        }
    }

    if (shouldSchedule) {
        QMetaObject::invokeMethod(this, "processLatestFrame", Qt::QueuedConnection);
    }
}

void DetectTask::processLatestFrame()
{
    if (!m_threadLogged) {
        qDebug() << "DetectTask::processLatestFrame thread:" << QThread::currentThreadId()
                 << "channel:" << m_channelId;
        m_threadLogged = true;
    }

    while (true) {
        QImage frame;
        qint64 frameId = -1;
        {
            QMutexLocker locker(&m_frameMutex);
            if (!m_detectEnable || !m_hasPendingFrame) {
                m_processScheduled = false;
                return;
            }

            frame = m_pendingFrame;
            frameId = m_pendingFrameId;
            m_pendingFrame = QImage();
            m_pendingFrameId = -1;
            m_hasPendingFrame = false;
        }

        const cv::Mat cvFrame = qImageToCvMat(frame);
        if (cvFrame.empty()) {
            continue;
        }

        emit sendDetectionResult(m_channelId, frameId, detectObjects(cvFrame));
    }
}

DetectionBoxes DetectTask::detectObjects(const cv::Mat &cvFrame)
{
    DetectionBoxes boxes;
    if (cvFrame.empty() || !ensureDetector()) {
        return boxes;
    }

    std::vector<YOLO_OUT> outputs;
    try {
        m_detector->detect(cvFrame, outputs);
    } catch (const cv::Exception &e) {
        qDebug() << "DetectTask: YOLOv11 detect failed:" << QString::fromLocal8Bit(e.what());
        return boxes;
    }

    for (const YOLO_OUT &output : outputs) {
        if (!isTargetClass(output.classId)) {
            continue;
        }

        DetectionBox box;
        box.rect = boundedRect(QRect(output.outRect.x, output.outRect.y, output.outRect.width, output.outRect.height),
                               cvFrame.size());
        if (box.rect.width() <= 2 || box.rect.height() <= 2) {
            continue;
        }
        box.classId = output.classId;
        box.confidence = output.score;
        box.className = cocoClassName(output.classId);
        boxes.append(box);
    }

    if (m_debugFramesLeft > 0) {
        qDebug() << "DetectTask: channel" << m_channelId
                 << "raw boxes:" << outputs.size()
                 << "target boxes:" << boxes.size();
        --m_debugFramesLeft;
    }

    return boxes;
}

void DetectTask::setDetectEnable(bool enable)
{
    QMutexLocker locker(&m_frameMutex);
    m_detectEnable = enable;
    if (!m_detectEnable) {
        m_pendingFrame = QImage();
        m_pendingFrameId = -1;
        m_hasPendingFrame = false;
    }
}

void DetectTask::releaseNet()
{
    QMutexLocker locker(&m_frameMutex);
    m_pendingFrame = QImage();
    m_pendingFrameId = -1;
    m_hasPendingFrame = false;
    m_processScheduled = false;
    m_detector.reset();
}
