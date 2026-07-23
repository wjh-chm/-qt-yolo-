#include "selectrl.h"

selectrl::selectrl(QWidget *parent) : QWidget(parent)
{
    setWindowTitle(QStringLiteral("选择类控件演示窗口"));
    resize(800, 600);
    setFont(QFont(QStringLiteral("宋体"), 16, QFont::Bold));

    QVBoxLayout *main_layout = new QVBoxLayout;
    setLayout(main_layout);

    // 这个小控件刻意保持简单，只演示单选按钮分组布局。
    QHBoxLayout *h_layout = new QHBoxLayout;
    main_layout->addLayout(h_layout);
    QLabel *lab_select = new QLabel(QStringLiteral("性别"), this);
    QRadioButton *rbMale = new QRadioButton(QStringLiteral("男"), this);
    QRadioButton *rbfeMale = new QRadioButton(QStringLiteral("女"), this);
    QRadioButton *rbOther = new QRadioButton(QStringLiteral("其他"), this);
    rbMale->setChecked(true);
    h_layout->addWidget(lab_select);
    h_layout->addWidget(rbMale);
    h_layout->addWidget(rbfeMale);
    h_layout->addWidget(rbOther);
    h_layout->addStretch();
}
