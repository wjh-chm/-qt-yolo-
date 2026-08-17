#ifndef REGISTERWIDGET_H
#define REGISTERWIDGET_H

#include <QBrush>
#include <QDebug>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPalette>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

// 预留的注册页面，当前版本仅保留界面骨架，便于后续扩展。
class registerwidget : public QWidget
{
    Q_OBJECT
public:
    explicit registerwidget(QWidget *parent = nullptr);

signals:

public slots:
};

#endif // REGISTERWIDGET_H
