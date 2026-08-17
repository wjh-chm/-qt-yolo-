#include "../include/Detect.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr int kBoxChannelCount = 64;
constexpr int kTargetClassIds[] = {0, 2}; // COCO: 0 = person, 2 = car

float dflDistance(const float* data, int area, int index, int side)
{
    constexpr int regMax = 16;
    const int baseChannel = side * regMax;
    float maxValue = data[(baseChannel * area) + index];

    for (int i = 1; i < regMax; ++i) {
        maxValue = std::max(maxValue, data[((baseChannel + i) * area) + index]);
    }

    float sum = 0.0f;
    float weighted = 0.0f;
    for (int i = 0; i < regMax; ++i) {
        const float value = std::exp(data[((baseChannel + i) * area) + index] - maxValue);
        sum += value;
        weighted += value * static_cast<float>(i);
    }

    return sum > 0.0f ? weighted / sum : 0.0f;
}
}

Detect::Detect(const std::string& modelPath)
    : YOLO(modelPath)
{
}

Detect::~Detect()
{
}

void Detect::yolov8_process(cv::Mat& out,
                            int img_w,
                            std::vector<cv::Rect>& bboxes,
                            std::vector<float>& scores,
                            std::vector<int>& classes)
{
    if (out.empty() || out.size[0] != 1) {
        return;
    }

    if (out.dims == 4) {
        const int channels = out.size[1];
        const int featureH = out.size[2];
        const int featureW = out.size[3];
        const int area = featureH * featureW;
        if (channels < kBoxChannelCount + 1 || area <= 0) {
            return;
        }

        const float strideX = static_cast<float>(model_input_size.width) / static_cast<float>(featureW);
        const float strideY = static_cast<float>(model_input_size.height) / static_cast<float>(featureH);
        const float xFactor = static_cast<float>(org_Size.width) / static_cast<float>(model_input_size.width);
        const float yFactor = static_cast<float>(org_Size.height) / static_cast<float>(model_input_size.height);
        const cv::Rect frameRect(0, 0, org_Size.width, org_Size.height);
        const float* data = reinterpret_cast<const float*>(out.data);

        for (int y = 0; y < featureH; ++y) {
            for (int x = 0; x < featureW; ++x) {
                const int index = y * featureW + x;

                int bestClassId = -1;
                float bestScore = 0.0f;
                for (const int classId : kTargetClassIds) {
                    const int classChannel = kBoxChannelCount + classId;
                    if (classChannel >= channels) {
                        continue;
                    }

                    const float classScore = sigmoid(data[classChannel * area + index]);
                    if (classScore > bestScore) {
                        bestScore = classScore;
                        bestClassId = classId;
                    }
                }

                if (bestClassId < 0 || bestScore < confThreshold) {
                    continue;
                }

                const float left = dflDistance(data, area, index, 0) * strideX;
                const float top = dflDistance(data, area, index, 1) * strideY;
                const float right = dflDistance(data, area, index, 2) * strideX;
                const float bottom = dflDistance(data, area, index, 3) * strideY;
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

                bboxes.push_back(box);
                scores.push_back(bestScore);
                classes.push_back(bestClassId);
            }
        }

        (void) img_w;
        return;
    }

    if (out.dims != 3) {
        return;
    }

    const int dim1 = out.size[1];
    const int dim2 = out.size[2];
    if (dim1 < 5 || dim2 <= 0) {
        return;
    }

    const bool channelFirst = dim1 <= 256;
    const int channels = channelFirst ? dim1 : dim2;
    const int candidates = channelFirst ? dim2 : dim1;
    if (channels < 5 || candidates <= 0) {
        return;
    }

    const float xFactor = static_cast<float>(org_Size.width) / static_cast<float>(model_input_size.width);
    const float yFactor = static_cast<float>(org_Size.height) / static_cast<float>(model_input_size.height);
    const cv::Rect frameRect(0, 0, org_Size.width, org_Size.height);
    const float* data = reinterpret_cast<const float*>(out.data);

    auto valueAt = [data, channelFirst, channels, candidates](int candidate, int channel) {
        if (channelFirst) {
            return data[channel * candidates + candidate];
        }
        return data[candidate * channels + channel];
    };

    for (int i = 0; i < candidates; ++i) {
        int bestClassId = -1;
        float bestScore = 0.0f;
        for (const int classId : kTargetClassIds) {
            const int classChannel = 4 + classId;
            if (classChannel >= channels) {
                continue;
            }

            const float classScore = valueAt(i, classChannel);
            if (classScore > bestScore) {
                bestScore = classScore;
                bestClassId = classId;
            }
        }

        if (bestClassId < 0 || bestScore < confThreshold) {
            continue;
        }

        const float cx = valueAt(i, 0);
        const float cy = valueAt(i, 1);
        const float w = valueAt(i, 2);
        const float h = valueAt(i, 3);

        const int left = static_cast<int>((cx - w * 0.5f) * xFactor);
        const int top = static_cast<int>((cy - h * 0.5f) * yFactor);
        const int width = static_cast<int>(w * xFactor);
        const int height = static_cast<int>(h * yFactor);

        cv::Rect box(left, top, width, height);
        box &= frameRect;
        if (box.width <= 0 || box.height <= 0) {
            continue;
        }

        bboxes.push_back(box);
        scores.push_back(bestScore);
        classes.push_back(bestClassId);
    }

    (void) img_w;
}

void Detect::detect(const cv::Mat& frame, std::vector<YOLO_OUT>& yoloOut)
{
    yoloOut.clear();
    if (frame.empty()) {
        return;
    }

    if (model_input_size.width <= 0 || model_input_size.height <= 0) {
        model_input_size = cv::Size(640, 640);
    }
    org_Size = frame.size();

    cv::Mat blob;
    cv::dnn::blobFromImage(frame,
                           blob,
                           1.0 / 255.0,
                           model_input_size,
                           cv::Scalar(0, 0, 0),
                           true,
                           false);
    net.setInput(blob);

    std::vector<cv::Mat> outs;
    net.forward(outs, net.getUnconnectedOutLayersNames());

    std::vector<cv::Rect> bboxes;
    std::vector<float> scores;
    std::vector<int> classes;
    for (cv::Mat& out : outs) {
        yolov8_process(out, model_input_size.width, bboxes, scores, classes);
    }

    std::vector<int> indices;
    cv::dnn::NMSBoxes(bboxes, scores, confThreshold, nmsThreshold, indices);

    for (const int idx : indices) {
        YOLO_OUT item;
        item.outRect = bboxes[idx];
        item.score = scores[idx];
        item.classId = classes[idx];
        yoloOut.push_back(item);
    }
}

void Detect::draw(cv::Mat& image, std::vector<YOLO_OUT> yolo_out)
{
    for (const YOLO_OUT& out : yolo_out) {
        const int classId = std::clamp(out.classId, 0, static_cast<int>(classNames_coco80.size()) - 1);
        std::string text = classNames_coco80[classId] + " " + std::to_string(out.score).substr(0, 4);
        cv::rectangle(image, out.outRect, random_color(classId), 2, 8);
        cv::putText(image,
                    text,
                    cv::Point(out.outRect.x, std::max(0, out.outRect.y - 6)),
                    cv::FONT_HERSHEY_SIMPLEX,
                    0.5,
                    random_color(classId),
                    2,
                    8,
                    false);
    }
}
