#pragma once
#include <windows.h>
#include <vector>
#include "ProcessManage.h"

// �������ڹ��̺���
LRESULT CALLBACK ModuleWndProc(
    HWND hwnd,        // handle to window
    UINT uMsg,        // message identifier
    WPARAM wParam,    // first message parameter
    LPARAM lParam     // second message parameter
);

// ����ģ�鴰�ڴ�������
int ModuleWindow(HWND parentHwnd, DWORD processId, const ProcessManage::ProcessInfo processInfo);
