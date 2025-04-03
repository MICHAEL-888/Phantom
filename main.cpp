#pragma once
#include <iostream>
#include <windows.h>
#include <winuser.h>
#include <commctrl.h>
#include <string>
#include "ProcessManage.h"
#include "PrivilegeElevate.h"

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define STATUSBAR_ID_1 1000
#define TABCONTROL_ID_1 2000
#define LISTVIEW_ID_1 3000

//int main() {
//	PrivilegeElevate privilegeElevate;
//	//privilegeElevate.AdmintoSystem();
//	ProcessManage processManage;
//
//	system("pause");
//	return 0;
//}

int GetDPI() {
    // 此函数参考精易模块获取系统DPI
    HDC hDC;
    int DPI_A, DPI_B;
    hDC = GetDC(NULL);
    DPI_A = GetDeviceCaps(hDC, 118) / GetDeviceCaps(hDC, 8) * 100;
    DPI_B = GetDeviceCaps(hDC, 88) / 96 * 100;
    ReleaseDC(NULL, hDC);
    if (DPI_A == 100) {
        return DPI_B;
    }
    else if (DPI_B == 100) {
        return DPI_A;
    }
    else if (DPI_A == DPI_B) {
        return DPI_A;
    }
    else {
        return 0;
    }
}

void FillProcessListView(HWND hwndListView, const ProcessManage& processManager) {
    // 清空列表视图
    ListView_DeleteAllItems(hwndListView);

    // 获取进程列表
    const auto& processList = processManager.GetProcessList();

    // 添加每个进程的信息
    for (size_t i = 0; i < processList.size(); i++) {
        // 创建列表项
        LVITEM lvItem;
        ZeroMemory(&lvItem, sizeof(lvItem));

        // 设置第一列（进程名称）
        lvItem.mask = LVIF_TEXT | LVIF_PARAM;
        lvItem.iItem = static_cast<int>(i);
        lvItem.iSubItem = 0;
        lvItem.pszText = const_cast<LPWSTR>(processList[i].m_processName.c_str());
        lvItem.lParam = processList[i].m_pid; // 存储PID，以便以后引用

        // 插入列表项
        int index = ListView_InsertItem(hwndListView, &lvItem);

        // 设置第二列（进程ID）
        std::wstring pidStr = std::to_wstring(processList[i].m_pid);
        ListView_SetItemText(hwndListView, index, 1, const_cast<LPWSTR>(pidStr.c_str()));

        // 设置第三列（进程路径）
        ListView_SetItemText(hwndListView, index, 2, const_cast<LPWSTR>(processList[i].m_processPath.c_str()));

        
    }
}


LRESULT CALLBACK MainWndProc(
    HWND hwnd,        // handle to window
    UINT uMsg,        // message identifier
    WPARAM wParam,    // first message parameter
    LPARAM lParam)    // second message parameter
{
    static ProcessManage processManage;
    static HWND hwndStatus;
    static HWND hwndTab;
    static HFONT hFont;
    static HWND hwndListView;

    switch (uMsg)
    {
    case WM_CREATE:
        // 创建字体
        hFont = CreateFontW(
            19 * GetDPI() / 100,       // 字体高度
            0,                         // 字体宽度
            0,                         // 文字角度
            0,                         // 基线角度
            FW_NORMAL,                 // 字体粗细
            FALSE,                     // 斜体
            FALSE,                     // 下划线
            FALSE,                     // 删除线
            DEFAULT_CHARSET,           // 字符集
            OUT_TT_ONLY_PRECIS,        // 输出精度
            CLIP_DEFAULT_PRECIS,       // 裁剪精度
            CLEARTYPE_QUALITY,         // 输出质量
            DEFAULT_PITCH | FF_DONTCARE,  // 字体间距和族
            L"Segoe UI Variable Display");           // 字体名称

        // 状态栏 
		hwndStatus = CreateWindowEx(
			0, 
            STATUSCLASSNAME, 
            NULL,
			WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
			0, 
            0, 
            0, 
            0,
			hwnd, 
            (HMENU)STATUSBAR_ID_1, 
            GetModuleHandle(NULL), 
            NULL);

        // 选项卡
        hwndTab = CreateWindowEx(
            0, 
            WC_TABCONTROL, 
            L"",
            WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
            0, 
            0, 
            300, 
            200,
            hwnd, 
            (HMENU)TABCONTROL_ID_1,
            GetModuleHandle(NULL), 
            NULL);

        // Tab设置字体
        SendMessage(hwndTab, WM_SETFONT, (WPARAM)hFont, TRUE);

        TCITEM tie;
        tie.mask = TCIF_TEXT;
        tie.pszText = L"进程管理";
        TabCtrl_InsertItem(hwndTab, 0, &tie);

        tie.pszText = L"设置";
        TabCtrl_InsertItem(hwndTab, 1, &tie);

        // 获取客户区矩形
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);

        // 获取选项卡控件的区域
        RECT rcTab;
        GetClientRect(hwndTab, &rcTab);

        // 调整列表框的位置，使其位于选项卡的客户区内
        RECT rcTabClient;
        TabCtrl_GetItemRect(hwndTab, 0, &rcTabClient);

        // 创建第一个选项卡对应的列表框
        hwndListView = CreateWindowEx(
            WS_EX_CLIENTEDGE,
            WC_LISTVIEW,
            NULL,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LVS_REPORT,
            10, // 左边距
            rcTabClient.bottom + 4 + 10, // 顶部边距
            rcClient.right - 20, // 宽度
            rcClient.bottom - rcTabClient.bottom + 4 - 20, // 高度
            hwnd,
            (HMENU)LISTVIEW_ID_1,
            GetModuleHandle(NULL),
            NULL);

        ListView_SetExtendedListViewStyle(hwndListView, LVS_EX_GRIDLINES | 
            LVS_EX_FULLROWSELECT |  
            LVS_EX_DOUBLEBUFFER | 
            LVS_EX_HEADERDRAGDROP | 
            LVS_EX_HEADERINALLVIEWS |
            LVS_EX_LABELTIP);

        // 设置列标题
        LVCOLUMN lvColumn;
        lvColumn.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        lvColumn.cx = 300;
        lvColumn.pszText = L"进程名称";
        ListView_InsertColumn(hwndListView, 0, &lvColumn);

        lvColumn.cx = 150;
        lvColumn.pszText = L"进程ID";
        ListView_InsertColumn(hwndListView, 1, &lvColumn);

        lvColumn.cx = 600;
        lvColumn.pszText = L"进程路径";
        ListView_InsertColumn(hwndListView, 2, &lvColumn);

        
        FillProcessListView(hwndListView, processManage);



		return 0;

    case WM_PAINT:
        // Paint the window's client area. 
        return DefWindowProc(hwnd, uMsg, wParam, lParam);

    case WM_SIZE:
        // Set the size and position of the window. 
        SendMessage(hwndStatus, WM_SIZE, 0, 0); // 使控件可变大小

        if (hwndTab)
        {
            RECT rcClient;  // 窗口客户区坐标，左上和右下
            GetClientRect(hwnd, &rcClient);
            SetWindowPos(hwndTab, NULL, 0, 0, rcClient.right, rcClient.bottom, SWP_NOZORDER);
        }

        // 调整列表视图的位置
        if (hwndTab && hwndListView)
        {
            RECT rcClient;  // 窗口客户区坐标，左上和右下
            GetClientRect(hwnd, &rcClient);
            // 获取选项卡的尺寸信息
            RECT rcTabClient;
            TabCtrl_GetItemRect(hwndTab, 0, &rcTabClient);
            int tabHeight = rcTabClient.bottom + 4; // 选项卡标题高度加一点边距

            // 获取状态栏的高度
            RECT rcStatus;
            GetWindowRect(hwndStatus, &rcStatus);
            int statusHeight = rcStatus.bottom - rcStatus.top;

            // 设置列表视图的位置和大小
            SetWindowPos(hwndListView, NULL,
                10,                       // 左边距
                tabHeight + 10,           // 顶部边距
                rcClient.right - 20,      // 宽度
                rcClient.bottom - tabHeight- rcStatus.bottom + rcStatus.top - 20,  // 高度，留出底部间距
                SWP_NOZORDER);
        }

        return 0;

    case WM_NOTIFY:
        if (((LPNMHDR)lParam)->idFrom == TABCONTROL_ID_1 && ((LPNMHDR)lParam)->code == TCN_SELCHANGE)
        {
            int iPage = TabCtrl_GetCurSel(hwndTab);
            if (iPage == 0)
            {
                ShowWindow(hwndListView, SW_SHOW);
            }
            else
            {
                ShowWindow(hwndListView, SW_HIDE);
            }
        }
        return 0;
    case WM_DESTROY:
        // Clean up window-specific data objects. 
        // 清理字体资源
        if (hFont)
        {
            DeleteObject(hFont);
        }
        PostQuitMessage(0);
        return 0;

        // 
        // Process other messages. 
        // 

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{

    WNDCLASSEXW wc = { };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_SAVEBITS | CS_DROPSHADOW | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);  // 使用一个白色的画刷给背景上色
    wc.lpszClassName = L"Phantom_ARK";

    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(
        0,                              // Optional window styles.
        wc.lpszClassName,                     // Window class
        L"Phantom_ARK v1.0.0",    // Window text
        WS_OVERLAPPEDWINDOW,            // Window style 
        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

        NULL,       // Parent window    
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
    );

    if (hwnd == NULL)
    {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    // Run the message loop.

    MSG msg = { };
    BOOL bRet;
    while ((bRet = GetMessage(&msg, hwnd, 0, 0)) != 0)
    {
        if (bRet == -1) {
            return 0;
        }
        else
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
    }

    return 0;
}
