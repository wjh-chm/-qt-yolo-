#ifndef SELECTRL_H
#define SELECTRL_H

#include <QHBoxLayout>
#include <QLabel>
#include <QObject>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QWidget>

// 单选按钮布局演示用的小控件。
class selectrl : public QWidget
{
    Q_OBJECT
public:
    explicit selectrl(QWidget *parent = nullptr);

signals:

public slots:
};

#endif // SELECTRL_H
