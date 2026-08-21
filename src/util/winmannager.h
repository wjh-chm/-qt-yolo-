#ifndef WINMANNAGER_H
#define WINMANNAGER_H

#include "view/loginwidget.h"
#include "view/mainwidget.h"

class ClientApi;

class WinMannager
{
public:
    static WinMannager *getInstence();

    WinMannager(const WinMannager &) = delete;
    WinMannager &operator=(const WinMannager &) = delete;

    ClientApi *clientApi() const;

    MainWidget mainwidget;
    LoginWidget loginwidget;

private:
    WinMannager();

private:
    static WinMannager *instence;
    ClientApi *m_clientApi = nullptr;
};

#endif // WINMANNAGER_H
