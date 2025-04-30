#pragma once
#include <windows.h>
#include <winuser.h>
#include "ProcessManage.h"
#include <CommCtrl.h>
#include <string>

#define LISTVIEW_ID_1 3000

extern int GetDPI();

LRESULT CALLBACK ModuleWndProc(
	HWND hwnd,        // handle to window
	UINT uMsg,        // message identifier
	WPARAM wParam,    // first message parameter
	LPARAM lParam     // second message parameter
);

int ModuleWindow(HWND parentHwnd, DWORD processId, const ProcessManage::ProcessInfo processInfo) {
    WNDCLASSEXW wc = { };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_SAVEBITS | CS_DROPSHADOW | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = ModuleWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);  // 使用一个白色的画刷给背景上色
    wc.lpszClassName = L"Phantom_ARK_Module";  // 使用不同的类名避免冲突

    RegisterClassExW(&wc);

    // 存储传入的参数，以便在WM_CREATE中使用
    struct ModuleWindowParams {
        DWORD processId;
        ProcessManage::ProcessInfo processInfo;
    };

    ModuleWindowParams* params = new ModuleWindowParams;
    params->processId = processId;
    params->processInfo = processInfo;

    HWND hwnd = CreateWindowExW(
        0,                              // Optional window styles.
        wc.lpszClassName,               // Window class
        L"模块信息 - Phantom_ARK v1.0.0",    // Window text
        WS_OVERLAPPEDWINDOW,            // Window style 
        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, 1200 * GetDPI() / 100 * 0.75, 750 * GetDPI() / 100 * 0.75, // 设置初始大小

        NULL,       // 不设置父窗口
        NULL,       // Menu
        wc.hInstance,     // Instance handle
        params            // 传递参数
    );

    if (hwnd == NULL)
    {
        delete params;
        return 0;
    }

    // 获取屏幕分辨率
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // 计算居中位置
    int x = (screenWidth - 1200 * GetDPI() / 100 * 0.75) / 2;
    int y = (screenHeight - 750 * GetDPI() / 100 * 0.75) / 2;

    // 设置窗口居中
    SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);


    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    // 创建独立的消息循环
    MSG msg = { };
    BOOL bRet;
    while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0)
    {
        if (bRet == -1) {
            // 处理错误
            return 0;
        }
        else if (!IsDialogMessage(hwnd, &msg)) // 允许对话框键盘导航
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // 如果窗口已被销毁，退出消息循环
        if (!IsWindow(hwnd))
            break;
    }

    return 1;
}

LRESULT CALLBACK ModuleWndProc(
    HWND hwnd,        // handle to window
    UINT uMsg,        // message identifier
    WPARAM wParam,    // first message parameter
    LPARAM lParam)    // second message parameter
{
    static ProcessManage::ProcessInfo processInfo;
    static HFONT hFont;
    static HWND hwndListView;
    static DWORD currentProcessId = 0;

    static int statwidths[] = { 800, -1 };  // -1 表示延伸到窗口右边
    static std::wstring statusText;

    switch (uMsg)
    {
    case WM_CREATE:
    {
        // 获取传入的参数
        CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
        if (cs->lpCreateParams) {
            struct ModuleWindowParams {
                DWORD processId;
                ProcessManage::ProcessInfo processInfo;
            };

            ModuleWindowParams* params = (ModuleWindowParams*)cs->lpCreateParams;
            currentProcessId = params->processId;
			processInfo = params->processInfo; // 复制参数到成员变量

            // 使用参数后立即删除
            delete params;
        }

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

        // 获取客户区矩形
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);

        // 创建列表视图
        hwndListView = CreateWindowEx(
            WS_EX_CLIENTEDGE,
            WC_LISTVIEW,
            NULL,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LVS_REPORT,
            10, // 左边距
            10, // 顶部边距
            rcClient.right - 20, // 宽度
            rcClient.bottom - 20, // 高度
            hwnd,
            (HMENU)LISTVIEW_ID_1,
            GetModuleHandle(NULL),
            NULL);

        ListView_SetExtendedListViewStyle(hwndListView, LVS_EX_GRIDLINES |
            LVS_EX_FULLROWSELECT |
            LVS_EX_DOUBLEBUFFER |
            LVS_EX_HEADERDRAGDROP |
            LVS_EX_HEADERINALLVIEWS |
            LVS_EX_LABELTIP |
            LVS_EX_TRANSPARENTBKGND);

        // 设置列标题
        LVCOLUMN lvColumn;
        lvColumn.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;
        lvColumn.cx = 300;
        lvColumn.fmt = LVCFMT_LEFT;
        lvColumn.pszText = L"模块名称";
        ListView_InsertColumn(hwndListView, 0, &lvColumn);

        lvColumn.cx = 150;
        lvColumn.fmt = LVCFMT_LEFT;
        lvColumn.pszText = L"模块基址";
        ListView_InsertColumn(hwndListView, 1, &lvColumn);

        lvColumn.cx = 150;
        lvColumn.fmt = LVCFMT_LEFT;
        lvColumn.pszText = L"模块大小";
        ListView_InsertColumn(hwndListView, 2, &lvColumn);

        lvColumn.cx = 600;
        lvColumn.fmt = LVCFMT_LEFT;
        lvColumn.pszText = L"模块路径";
        ListView_InsertColumn(hwndListView, 3, &lvColumn);

        lvColumn.cx = 200;
        lvColumn.fmt = LVCFMT_LEFT;
        lvColumn.pszText = L"文件厂商";
        ListView_InsertColumn(hwndListView, 4, &lvColumn);

        lvColumn.cx = 300;
        lvColumn.fmt = LVCFMT_LEFT;
        lvColumn.pszText = L"文件描述";
        ListView_InsertColumn(hwndListView, 5, &lvColumn);

        // 在这里可以加载和填充当前进程ID的模块列表
        // FillModuleListView(hwndListView, currentProcessId);

        // 设置窗口标题显示当前进程ID
        std::wstring windowTitle = L"模块信息" + processInfo.getProcessName() 
            + L"[" + std::to_wstring(processInfo.getPid()) + L"] - Phantom_ARK";
        SetWindowText(hwnd, windowTitle.c_str());

        return 0;
    }

    case WM_PAINT:
        // Paint the window's client area. 
        return DefWindowProc(hwnd, uMsg, wParam, lParam);

    case WM_SIZE:
        // 调整列表视图大小以适应窗口
        if (hwndListView)
        {
            RECT rcClient;
            GetClientRect(hwnd, &rcClient);
            SetWindowPos(hwndListView, NULL,
                10, 10,
                rcClient.right - 20,
                rcClient.bottom - 20,
                SWP_NOZORDER);
        }
        return 0;

    case WM_NOTIFY:
        // 处理列表视图的通知消息
        return 0;

    case WM_COMMAND:
        // 处理命令消息
        return 0;

    case WM_CONTEXTMENU:
        // 这里可以添加模块列表的右键菜单
        return 0;


    case WM_DESTROY:
        // 清理字体资源
        if (hFont)
        {
            DeleteObject(hFont);
        }
        // 不调用PostQuitMessage(0)，因为我们只想关闭这个窗口，而非整个应用程序
        return 0;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}
