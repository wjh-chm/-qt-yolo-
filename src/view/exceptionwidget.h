#ifndef EXCEPTIONWIDGET_H
#define EXCEPTIONWIDGET_H

#include "reviewwidget.h"

class ExceptionWidget : public ReviewWidget
{
    Q_OBJECT

public:
    explicit ExceptionWidget(QWidget *parent = nullptr);
};

#endif // EXCEPTIONWIDGET_H
