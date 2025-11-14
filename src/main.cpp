#pragma once
#include <iostream>
#include <windows.h>
#include <winuser.h>
#include <commctrl.h>
#include <string>
#include <sstream>
#include <iomanip>
#include "ProcessManage.h"
#include "PrivilegeElevate.h"
#include "ModuleWindow.h"
#include "MainWindow.h"

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

extern int GetDPI();

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    MainWindow app;
    if (!app.Register(hInstance)) {
        return 0;
    }
    if (!app.Create(hInstance, nCmdShow)) {
        return 0;
    }

    MSG msg{};
    BOOL bRet;
    while ((bRet = GetMessage(&msg, nullptr, 0, 0)) != 0)
    {
        if (bRet == -1) {
            return 0;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
