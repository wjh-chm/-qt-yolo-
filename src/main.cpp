#include "view/mainwidget.h"

#include "util/winmannager.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    WinMannager::getInstence()->mainwidget.show();
    return app.exec();
}
