#ifndef VERIFYCODELABEL_H
#define VERIFYCODELABEL_H

#include <QColor>
#include <QLabel>
#include <QMouseEvent>
#include <QPaintEvent>

#include <random>

// 自绘验证码控件，点击后会重新生成验证码。
class VerifyCodeLabel : public QLabel
{
    Q_OBJECT
public:
    explicit VerifyCodeLabel(QWidget *parent = nullptr);

    // 返回当前验证码文本，供登录校验使用。
    QString getCode() const;

    // 生成新的验证码字符串，并触发重绘。
    void generateCode();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    QColor getRandomColor();
    int randomInt() const;

    QString m_code;
    const int codeLen = 4;
};

#endif // VERIFYCODELABEL_H
