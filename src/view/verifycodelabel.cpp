#include "verifycodelabel.h"

#include <QFont>
#include <QPainter>
#include <QTime>
#include <QtMath>

VerifyCodeLabel::VerifyCodeLabel(QWidget *parent)
    : QLabel(parent)
{
    setCursor(Qt::PointingHandCursor);
    generateCode();
}

QColor VerifyCodeLabel::getRandomColor()
{
    int r = randomInt() % 200;
    int g = randomInt() % 200;
    int b = randomInt() % 200;
    return QColor(r, g, b);
}

int VerifyCodeLabel::randomInt() const
{
    // 静态随机引擎避免每次调用都重新播种，同时保持足够随机。
    static std::mt19937 engine(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, RAND_MAX);
    return dist(engine);
}

void VerifyCodeLabel::generateCode()
{
    // 去掉容易混淆的字符，提升人工识别验证码的成功率。
    QString charPool = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
    m_code.clear();

    for (int i = 0; i < codeLen; ++i) {
        int index = randomInt() % charPool.size();
        m_code += charPool.at(index);
    }
    update();
}

QString VerifyCodeLabel::getCode() const
{
    return m_code;
}

void VerifyCodeLabel::paintEvent(QPaintEvent *event)
{
    QLabel::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.fillRect(rect(), QColor(240, 240, 240));

    // 画干扰线，增加验证码被直接 OCR 识别的难度。
    for (int i = 0; i < 6; ++i) {
        painter.setPen(getRandomColor());
        int x1 = randomInt() % width();
        int y1 = randomInt() % height();
        int x2 = randomInt() % width();
        int y2 = randomInt() % height();
        painter.drawLine(x1, y1, x2, y2);
    }

    // 画噪点，进一步增加背景扰动。
    for (int i = 0; i < 120; ++i) {
        painter.setPen(getRandomColor());
        int x = randomInt() % width();
        int y = randomInt() % height();
        painter.drawPoint(x, y);
    }

    QFont font("Microsoft YaHei", 22, QFont::Bold);
    painter.setFont(font);

    int w = width() / codeLen;
    for (int i = 0; i < codeLen; ++i) {
        painter.setPen(getRandomColor());

        // 每个字符做轻微旋转，让验证码更接近真实场景。
        painter.save();
        painter.translate(i * w + w / 2, height() / 2);
        painter.rotate((randomInt() % 30) - 15);
        painter.drawText(-w / 2, -height() / 3, w, height(), Qt::AlignCenter, m_code.at(i));
        painter.restore();
    }
}

void VerifyCodeLabel::mousePressEvent(QMouseEvent *event)
{
    QLabel::mousePressEvent(event);
    generateCode();
}
