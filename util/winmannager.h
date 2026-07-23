#ifndef WINMANNAGER_H
#define WINMANNAGER_H

#include "../view/loginwidget.h"
#include "../view/mainwidget.h"

// 演示程序顶层窗口的单例管理器。
class WinMannager
{
public:
    static WinMannager *getInstence();

    // 进程内窗口所有权必须唯一，因此禁止拷贝。
    WinMannager(const WinMannager &) = delete;
    WinMannager &operator=(const WinMannager &) = delete;

    MainWidget mainwidget;
    LoginWidget loginwidget;

private:
    WinMannager();
    static WinMannager *instence;
};

#endif // WINMANNAGER_H
