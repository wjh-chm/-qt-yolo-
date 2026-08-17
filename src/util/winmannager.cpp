#include "winmannager.h"

// 统一持有登录窗口和主窗口的全局管理对象。
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
}
