#include "view/mainwidget.h"
#include <QApplication>

#include "model/usermodel.h"
#include "util/winmannager.h"
// 程序入口：启动 Qt 事件循环，并显示主窗口。
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    WinMannager::getInstence()->mainwidget.show();

    return a.exec();
}
