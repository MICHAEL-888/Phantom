#pragma once
#include <windows.h>
#include <vector>
#include "ProcessManage.h"

// 声明窗口过程函数
LRESULT CALLBACK ModuleWndProc(
    HWND hwnd,        // handle to window
    UINT uMsg,        // message identifier
    WPARAM wParam,    // first message parameter
    LPARAM lParam     // second message parameter
);

// 声明模块窗口创建函数
int ModuleWindow(HWND parentHwnd, DWORD processId, const ProcessManage::ProcessInfo processInfo);
