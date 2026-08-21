#include "winmannager.h"

#include "net/clientapi.h"

WinMannager *WinMannager::instence = nullptr;

WinMannager *WinMannager::getInstence()
{
    if (WinMannager::instence == nullptr) {
        WinMannager::instence = new WinMannager;
    }
    return WinMannager::instence;
}

WinMannager::WinMannager()
{
    m_clientApi = new ClientApi(&mainwidget);
    loginwidget.setClientApi(m_clientApi);
    mainwidget.setClientApi(m_clientApi);
}

ClientApi *WinMannager::clientApi() const
{
    return m_clientApi;
}
