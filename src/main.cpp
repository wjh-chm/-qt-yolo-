#include "view/mainwidget.h"

#include "net/clientapi.h"
#include "util/winmannager.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    WinMannager *winMannager = WinMannager::getInstence();
    winMannager->mainwidget.show();
    winMannager->clientApi()->connectToServer("192.168.227.128", 9000);

    return app.exec();
}
