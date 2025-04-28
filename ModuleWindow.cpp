#pragma once
#include <windows.h>
#include <winuser.h>
#include "ProcessManage.h"
#include <CommCtrl.h>

#define LISTVIEW_ID_1 3000

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

int ModuleWindow(HWND parentHwnd, DWORD processId, std::vector<ProcessManage::ProcessInfo> m_processList) {
	WNDCLASSEXW wc = { };
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_SAVEBITS | CS_DROPSHADOW | CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = ModuleWndProc;
	wc.hInstance = GetModuleHandle(NULL);
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
		wc.hInstance,  // Instance handle
		NULL        // Additional application data
	);

	if (hwnd == NULL)
	{
		return 0;
	}

	ShowWindow(hwnd, SW_SHOW);

	return 0;
}

LRESULT CALLBACK ModuleWndProc(
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
	static HWND hwndElevateButton;
	static HWND hwndRefreshButton;


	static int statwidths[] = { 800, -1 };  // -1 表示延伸到窗口右边
	static std::wstring statusText;

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
			LVS_EX_LABELTIP |
			LVS_EX_TRANSPARENTBKGND);

		// 设置列标题
		LVCOLUMN lvColumn;
		lvColumn.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;
		lvColumn.cx = 300;
		lvColumn.fmt = LVCFMT_LEFT;
		lvColumn.pszText = L"进程名称";
		ListView_InsertColumn(hwndListView, 0, &lvColumn);

		lvColumn.cx = 150;
		lvColumn.fmt = LVCFMT_LEFT;
		lvColumn.pszText = L"进程ID";
		ListView_InsertColumn(hwndListView, 1, &lvColumn);

		lvColumn.cx = 600;
		lvColumn.fmt = LVCFMT_LEFT;
		lvColumn.pszText = L"映像路径";
		ListView_InsertColumn(hwndListView, 2, &lvColumn);

		lvColumn.cx = 180;
		lvColumn.fmt = LVCFMT_CENTER;  // 设置居中对齐
		lvColumn.pszText = L"隐藏进程检测";
		ListView_InsertColumn(hwndListView, 3, &lvColumn);

		lvColumn.cx = 150;
		lvColumn.fmt = LVCFMT_LEFT;
		lvColumn.pszText = L"父进程ID";
		ListView_InsertColumn(hwndListView, 4, &lvColumn);

		lvColumn.cx = 300;
		lvColumn.fmt = LVCFMT_LEFT;
		lvColumn.pszText = L"父进程名";
		ListView_InsertColumn(hwndListView, 5, &lvColumn);

		lvColumn.cx = 180;
		lvColumn.fmt = LVCFMT_CENTER;  // 设置居中对齐
		lvColumn.pszText = L"系统关键进程";
		ListView_InsertColumn(hwndListView, 6, &lvColumn);

		lvColumn.cx = 300;
		lvColumn.fmt = LVCFMT_LEFT;
		lvColumn.pszText = L"进程保护";
		ListView_InsertColumn(hwndListView, 7, &lvColumn);

		lvColumn.cx = 350;
		lvColumn.fmt = LVCFMT_LEFT;
		lvColumn.pszText = L"用户名";
		ListView_InsertColumn(hwndListView, 8, &lvColumn);

		lvColumn.cx = 300;
		lvColumn.fmt = LVCFMT_LEFT;
		lvColumn.pszText = L"PEB基址";
		ListView_InsertColumn(hwndListView, 9, &lvColumn);

		FillProcessListView(hwndListView, processManage);

		// 设置按钮字体
		SendMessage(hwndElevateButton, WM_SETFONT, (WPARAM)hFont, TRUE);

		// 初始隐藏按钮（因为默认显示第一个选项卡）
		ShowWindow(hwndElevateButton, SW_HIDE);

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
				rcClient.bottom - tabHeight - rcStatus.bottom + rcStatus.top - 20,  // 高度，留出底部间距
				SWP_NOZORDER);
		}


		return 0;

	case WM_NOTIFY:



		return 0;
	case WM_COMMAND:

	case WM_CONTEXTMENU:
	
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