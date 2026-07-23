#include "detectask.h"

#include <QDebug>
#include <QThread>

#include <algorithm>
#include <cmath>

namespace
{
float sigmoid(float value)
{
    return 1.0f / (1.0f + std::exp(-value));
}

float dflDistance(const float *data, int side, int index, int area)
{
    const int regMax = 16;
    const int baseChannel = side * regMax;
    float maxValue = data[(baseChannel * area) + index];

    for (int i = 1; i < regMax; ++i) {
        maxValue = std::max(maxValue, data[((baseChannel + i) * area) + index]);
    }

    float sum = 0.0f;
    float weighted = 0.0f;

    for (int i = 0; i < regMax; ++i) {
        const float score = std::exp(data[((baseChannel + i) * area) + index] - maxValue);
        sum += score;
        weighted += score * static_cast<float>(i);
    }

    return weighted / sum;
}

cv::Mat qImageToCvMat(const QImage &image)
{
    if (image.isNull()) {
        return cv::Mat();
    }

    QImage converted = image.convertToFormat(QImage::Format_RGB888);
    cv::Mat mat(converted.height(),
                converted.width(),
                CV_8UC3,
                const_cast<uchar *>(converted.bits()),
                converted.bytesPerLine());
    cv::Mat bgrMat;
    cv::cvtColor(mat, bgrMat, cv::COLOR_RGB2BGR);
    return bgrMat;
}

QImage cvMatToQImage(const cv::Mat &mat)
{
    if (mat.empty()) {
        return QImage();
    }

    cv::Mat rgbMat;
    if (mat.channels() == 3) {
        cv::cvtColor(mat, rgbMat, cv::COLOR_BGR2RGB);
        return QImage(rgbMat.data,
                      rgbMat.cols,
                      rgbMat.rows,
                      static_cast<int>(rgbMat.step),
                      QImage::Format_RGB888)
            .copy();
    }

    if (mat.channels() == 4) {
        cv::cvtColor(mat, rgbMat, cv::COLOR_BGRA2RGBA);
        return QImage(rgbMat.data,
                      rgbMat.cols,
                      rgbMat.rows,
                      static_cast<int>(rgbMat.step),
                      QImage::Format_RGBA8888)
            .copy();
    }

    return QImage();
}
}

DetectTask::DetectTask(int channelId, QObject *parent)
    : QObject(parent),
      m_channelId(channelId)
{
    m_yoloNet = cv::dnn::readNet("D:/qt6code/yolov8/weights/yolov8n-face.onnx");
    m_yoloNet.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
    m_yoloNet.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);

    if (m_yoloNet.empty()) {
        qDebug() << "DetectTask: YOLO模型加载失败";
    }
}

void DetectTask::recvFrame(int channelId, const QImage &frame)
{
    if (channelId != m_channelId) {
        return;
    }

    if (!m_threadLogged) {
        qDebug() << "DetectTask::recvFrame thread:" << QThread::currentThreadId()
                 << "channel:" << channelId;
        m_threadLogged = true;
    }

    if (!m_detectEnable || frame.isNull()) {
        emit sendDrawFrame(channelId, frame);
        return;
    }

    cv::Mat cvFrame = qImageToCvMat(frame);
    if (cvFrame.empty()) {
        emit sendDrawFrame(channelId, frame);
        return;
    }

    cv::Mat inputBlob = cv::dnn::blobFromImage(cvFrame,
                                               1.0 / 255.0,
                                               cv::Size(m_inputSize, m_inputSize),
                                               cv::Scalar(),
                                               true,
                                               false);
    m_yoloNet.setInput(inputBlob);

    const bool debugThisFrame = m_debugFramesLeft > 0;
    std::vector<cv::String> outputNames = m_yoloNet.getUnconnectedOutLayersNames();
    std::vector<cv::Mat> outputs;
    m_yoloNet.forward(outputs, outputNames);

    int selectedOutput = -1;
    for (size_t outIdx = 0; outIdx < outputs.size(); ++outIdx) {
        const cv::Mat &out = outputs[outIdx];

        if (debugThisFrame) {
            const QString name = outIdx < outputNames.size()
                ? QString::fromStdString(outputNames[outIdx])
                : QStringLiteral("unknown");
            qDebug() << "YOLO output" << static_cast<int>(outIdx) << "name:" << name << "dims:" << out.dims;
            for (int i = 0; i < out.dims; ++i) {
                qDebug() << "YOLO output" << static_cast<int>(outIdx) << "size[" << i << "]:" << out.size[i];
            }
        }

        if (selectedOutput < 0 && (out.dims == 2 || out.dims == 3)) {
            selectedOutput = static_cast<int>(outIdx);
        }
    }

    std::vector<cv::Rect> faceBoxes;
    std::vector<float> confs;
    const float xFactor = static_cast<float>(cvFrame.cols) / static_cast<float>(m_inputSize);
    const float yFactor = static_cast<float>(cvFrame.rows) / static_cast<float>(m_inputSize);
    const cv::Rect frameRect(0, 0, cvFrame.cols, cvFrame.rows);
    float maxConf = 0.0f;

    if (selectedOutput < 0) {
        for (const cv::Mat &output : outputs) {
            if (output.dims != 4 || output.size[0] != 1 || output.size[1] < 65) {
                continue;
            }

            const int channels = output.size[1];
            const int featureH = output.size[2];
            const int featureW = output.size[3];
            const int area = featureH * featureW;
            const float strideX = static_cast<float>(m_inputSize) / static_cast<float>(featureW);
            const float strideY = static_cast<float>(m_inputSize) / static_cast<float>(featureH);
            const float *data = output.ptr<float>();

            for (int y = 0; y < featureH; ++y) {
                for (int x = 0; x < featureW; ++x) {
                    const int index = y * featureW + x;
                    const float conf = sigmoid(data[64 * area + index]);
                    maxConf = std::max(maxConf, conf);

                    if (conf < m_confThresh) {
                        continue;
                    }

                    const float left = dflDistance(data, 0, index, area) * strideX;
                    const float top = dflDistance(data, 1, index, area) * strideY;
                    const float right = dflDistance(data, 2, index, area) * strideX;
                    const float bottom = dflDistance(data, 3, index, area) * strideY;
                    const float centerX = (static_cast<float>(x) + 0.5f) * strideX;
                    const float centerY = (static_cast<float>(y) + 0.5f) * strideY;

                    const int x1 = static_cast<int>((centerX - left) * xFactor);
                    const int y1 = static_cast<int>((centerY - top) * yFactor);
                    const int x2 = static_cast<int>((centerX + right) * xFactor);
                    const int y2 = static_cast<int>((centerY + bottom) * yFactor);
                    cv::Rect box(x1, y1, x2 - x1, y2 - y1);
                    box &= frameRect;

                    if (box.width <= 0 || box.height <= 0) {
                        continue;
                    }

                    faceBoxes.emplace_back(box);
                    confs.push_back(conf);
                }
            }

            if (debugThisFrame) {
                qDebug() << "YOLO raw output channels:" << channels
                         << "feature:" << featureW << "x" << featureH;
            }
        }

        if (debugThisFrame) {
            qDebug() << "YOLO raw max confidence:" << maxConf;
            qDebug() << "YOLO raw boxes before NMS:" << faceBoxes.size();
            --m_debugFramesLeft;
        }

        std::vector<int> nmsIdx;
        cv::dnn::NMSBoxes(faceBoxes, confs, m_confThresh, 0.45f, nmsIdx);
        cv::Mat drawFrame = cvFrame.clone();

        if (debugThisFrame) {
            qDebug() << "YOLO raw boxes after NMS:" << nmsIdx.size();
            if (!faceBoxes.empty()) {
                qDebug() << "YOLO raw first box:" << faceBoxes.front().x << faceBoxes.front().y
                         << faceBoxes.front().width << faceBoxes.front().height;
            }
        }

        for (int idx : nmsIdx) {
            cv::rectangle(drawFrame, faceBoxes[idx], cv::Scalar(0, 0, 255), 2);
        }

        emit sendDrawFrame(channelId, cvMatToQImage(drawFrame));
        return;
    }

    cv::Mat output = outputs[selectedOutput];
    cv::Mat detections;
    int rows = 0;
    int dimensions = 0;

    if (output.dims == 3) {
        if (output.size[1] < output.size[2]) {
            dimensions = output.size[1];
            rows = output.size[2];
            cv::Mat raw(dimensions, rows, CV_32F, output.ptr<float>());
            cv::transpose(raw, detections);
        } else {
            rows = output.size[1];
            dimensions = output.size[2];
            detections = cv::Mat(rows, dimensions, CV_32F, output.ptr<float>());
        }
    } else if (output.dims == 2) {
        if (output.rows < output.cols && output.rows <= 100) {
            dimensions = output.rows;
            rows = output.cols;
            cv::transpose(output, detections);
        } else {
            rows = output.rows;
            dimensions = output.cols;
            detections = output;
        }
    } else {
        emit sendDrawFrame(channelId, frame);
        return;
    }

    if (dimensions < 5) {
        emit sendDrawFrame(channelId, frame);
        return;
    }

    if (debugThisFrame) {
        qDebug() << "YOLO rows:" << rows << "dimensions:" << dimensions;
    }

    for (int i = 0; i < rows; ++i) {
        const float *data = detections.ptr<float>(i);
        float conf = data[4];

        if (dimensions > 20) {
            conf = 0.0f;
            for (int j = 4; j < dimensions; ++j) {
                conf = std::max(conf, data[j]);
            }
        }

        maxConf = std::max(maxConf, conf);

        if (conf >= m_confThresh) {
            float cx = data[0];
            float cy = data[1];
            float w = data[2];
            float h = data[3];

            const bool normalized = cx <= 1.5f && cy <= 1.5f && w <= 1.5f && h <= 1.5f;
            if (normalized) {
                cx *= cvFrame.cols;
                cy *= cvFrame.rows;
                w *= cvFrame.cols;
                h *= cvFrame.rows;
            } else {
                cx *= xFactor;
                cy *= yFactor;
                w *= xFactor;
                h *= yFactor;
            }

            const int x = static_cast<int>(cx - w / 2);
            const int y = static_cast<int>(cy - h / 2);
            cv::Rect box(x, y, static_cast<int>(w), static_cast<int>(h));
            box &= frameRect;

            if (box.width <= 0 || box.height <= 0) {
                continue;
            }

            faceBoxes.emplace_back(box);
            confs.push_back(conf);
        }
    }

    std::vector<int> nmsIdx;
    cv::dnn::NMSBoxes(faceBoxes, confs, m_confThresh, 0.45f, nmsIdx);
    cv::Mat drawFrame = cvFrame.clone();
    if (debugThisFrame) {
        qDebug() << "YOLO max confidence:" << maxConf;
        qDebug() << "YOLO boxes before NMS:" << faceBoxes.size() << "after NMS:" << nmsIdx.size();
        if (!faceBoxes.empty()) {
            qDebug() << "YOLO first box:" << faceBoxes.front().x << faceBoxes.front().y
                     << faceBoxes.front().width << faceBoxes.front().height;
        }
        --m_debugFramesLeft;
    }

    for (int idx : nmsIdx) {
        cv::rectangle(drawFrame, faceBoxes[idx], cv::Scalar(0, 0, 255), 2);
    }

    emit sendDrawFrame(channelId, cvMatToQImage(drawFrame));
}

void DetectTask::setDetectEnable(bool enable)
{
    m_detectEnable = enable;
}

void DetectTask::releaseNet()
{
    m_yoloNet = cv::dnn::Net();
}
