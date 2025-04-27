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

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define STATUSBAR_ID_1 1000
#define TABCONTROL_ID_1 2000
#define LISTVIEW_ID_1 3000
#define BUTTON_ID_1 4000
#define ID_MENU_REFRESH    5001
#define ID_MENU_TERMINATE_PROCESS  5002
#define ID_MENU_PROPERTY   5003
#define ID_MENU_TERMINATE_PROCESSTREE 5004
#define ID_MENU_VIEW_PROCESS_MODULES 5005



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

static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	int column = LOWORD(lParamSort);    // 列号
	bool ascending = HIWORD(lParamSort); // 排序方向

	HWND hList = GetDlgItem(GetActiveWindow(), LISTVIEW_ID_1);
	WCHAR szText1[256]{};
	WCHAR szText2[256]{};

	// 获取要比较的两个项的文本
	int index1 = -1, index2 = -1;

	// 找到对应PID的项的索引
	for (int i = 0; i < ListView_GetItemCount(hList); i++) {
		LVITEM lvi{};
		lvi.mask = LVIF_PARAM;
		lvi.iItem = i;
		ListView_GetItem(hList, &lvi);
		if (lvi.lParam == lParam1) index1 = i;
		if (lvi.lParam == lParam2) index2 = i;
		if (index1 != -1 && index2 != -1) break;
	}

	if (index1 == -1 || index2 == -1) return 0;

	// 获取文本进行比较
	ListView_GetItemText(hList, index1, column, szText1, _countof(szText1));
	ListView_GetItemText(hList, index2, column, szText2, _countof(szText2));

	ULONG result;
	// 根据不同列使用不同的比较方法
	switch (column)
	{
	case 1: // PID列 - 直接比较lParam
		result = (ULONG)(lParam1 - lParam2);
		break;
	case 4: // 父进程ID列 - 数字比较
	{
		ULONG num1 = _wtoi(szText1);
		ULONG num2 = _wtoi(szText2);
		result = num1 - num2;
		break;
	}
	default: // 其他列 - 字符串比较
		result = _wcsicmp(szText1, szText2);
	}

	return ascending ? result : -result;
}





void FillProcessListView(HWND hwndListView, const ProcessManage& processManager) {
	// 禁用重绘
	SendMessage(hwndListView, WM_SETREDRAW, FALSE, 0);

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

		// 设置第四列（隐藏进程检测）
		std::wstring isHideStr = processList[i].m_isHide ? L"true" : L"";
		ListView_SetItemText(hwndListView, index, 3, const_cast<LPWSTR>(isHideStr.c_str()));

		// 设置第五列（父进程ID）
		std::wstring parrentPidStr = std::to_wstring(processList[i].m_parrentProcessId);
		ListView_SetItemText(hwndListView, index, 4, const_cast<LPWSTR>(parrentPidStr.c_str()));

		// 设置第六列（父进程名）
		ListView_SetItemText(hwndListView, index, 5, const_cast<LPWSTR>(processList[i].m_parrentProcessName.c_str()));

		// 设置第七列（关键进程）
		std::wstring isCriticalStr = processList[i].m_isCritical ? L"true" : L"";
		ListView_SetItemText(hwndListView, index, 6, const_cast<LPWSTR>(isCriticalStr.c_str()));

		// 设置第八列（PPL）
		if ((processList[i].m_ppl & 0b00000111) != 0) {
			std::wstring type{};
			std::wstring signer{};

			if ((processList[i].m_ppl & 0b00000111) == 1)	type = L"Light";
			else if ((processList[i].m_ppl & 0b00000111) == 2)	type = L"Full";

			if ((processList[i].m_ppl & 0b11110000) == 0) signer = L"(None)";
			if ((processList[i].m_ppl & 0b11110000) == 16) signer = L"(Authenticode)";
			if ((processList[i].m_ppl & 0b11110000) == 32) signer = L"(CodeGen)";
			if ((processList[i].m_ppl & 0b11110000) == 48) signer = L"(Antimalware)";
			if ((processList[i].m_ppl & 0b11110000) == 64) signer = L"(Lsa)";
			if ((processList[i].m_ppl & 0b11110000) == 80) signer = L"(Windows)";
			if ((processList[i].m_ppl & 0b11110000) == 96) signer = L"(WinTcb)";
			if ((processList[i].m_ppl & 0b11110000) == 112) signer = L"(WinSystem)";
			if ((processList[i].m_ppl & 0b11110000) == 128) signer = L"(App)";
			if ((processList[i].m_ppl & 0b11110000) == 144) signer = L"(Max)";

			std::wstring tmp = type + L" " + signer;
			ListView_SetItemText(hwndListView, index, 7, const_cast<LPWSTR>(tmp.c_str()));
		}

		// 设置第九列（用户名）
		if (!processList[i].m_userDomain.empty() && !processList[i].m_userName.empty()) {
			std::wstring str9 = processList[i].m_userDomain + L"\\" + processList[i].m_userName;
			ListView_SetItemText(hwndListView, index, 8, const_cast<LPWSTR>(str9.c_str()));
		}
		
		// 设置第十列（PEB基址）
		if (processList[i].m_peb != nullptr) {
			std::wstringstream wss10;
			wss10 << L"0x" << std::uppercase << std::hex 
				<< reinterpret_cast<uintptr_t>(processList[i].m_peb);
			std::wstring str10 = wss10.str();
			ListView_SetItemText(hwndListView, index, 9, const_cast<LPWSTR>(str10.c_str()));
		}
		
		

	}

	// 开启重绘
	SendMessage(hwndListView, WM_SETREDRAW, TRUE, 0);

	// 强制重绘列表视图和表头
	InvalidateRect(hwndListView, NULL, TRUE);

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

		// 设置状态栏分区

		SendMessage(hwndStatus, SB_SETPARTS, 2, (LPARAM)statwidths);

		// 设置初始状态栏文本

		statusText = L"进程数量：" + std::to_wstring(processManage.GetProcessCount())
			+ L"    检测到隐藏进程：" + std::to_wstring(processManage.GetProcessHiddenCount());
		SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)statusText.c_str());
		SendMessage(hwndStatus, SB_SETTEXT, 1, (LPARAM)L"Phantom_ARK v1.0.0");


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

		hwndElevateButton = CreateWindowEx(
			0,
			L"BUTTON",
			L"提升至SYSTEM权限",
			WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			rcTabClient.right - 100,  // x位置
			rcTabClient.bottom - 4 - 10,  // y位置
			200 * GetDPI() / 100,  // 宽度
			40 * GetDPI() / 100,   // 高度
			hwnd,
			(HMENU)BUTTON_ID_1,
			GetModuleHandle(NULL),
			NULL
		);

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

		// 调整提权按钮位置
		if (hwndTab && hwndElevateButton)
		{
			RECT rcClient;
			GetClientRect(hwnd, &rcClient);
			RECT rcTabClient;
			TabCtrl_GetItemRect(hwndTab, 0, &rcTabClient);
			int tabHeight = rcTabClient.bottom + 4;

			// 按钮位置调整
			SetWindowPos(hwndElevateButton, NULL,
				10,                    // x位置
				tabHeight + 10,        // y位置
				400,                   // 宽度
				200,                    // 高度
				SWP_NOZORDER);
		}

		return 0;

	case WM_NOTIFY:
		if (((LPNMHDR)lParam)->hwndFrom == ListView_GetHeader(hwndListView))
		{
			if (((LPNMHDR)lParam)->code == HDN_ITEMCLICK)
			{
				LPNMHEADER phdr = (LPNMHEADER)lParam;
				// 获取点击的列索引
				int column = phdr->iItem;
				static bool ascending = true;  // 排序方向

				SendMessage(hwndListView, WM_SETREDRAW, FALSE, 0);
				// 对列表项进行排序
				ListView_SortItems(hwndListView, CompareFunc, MAKELPARAM(column, ascending));
				ascending = !ascending;  // 切换排序方向
				SendMessage(hwndListView, WM_SETREDRAW, TRUE, 0);

				return 0;
			}
		}

		if (((LPNMHDR)lParam)->idFrom == TABCONTROL_ID_1 && ((LPNMHDR)lParam)->code == TCN_SELCHANGE)
		{
			int iPage = TabCtrl_GetCurSel(hwndTab);
			if (iPage == 0)
			{
				ShowWindow(hwndListView, SW_SHOW);
				ShowWindow(hwndElevateButton, SW_HIDE);
			}
			else
			{
				ShowWindow(hwndListView, SW_HIDE);
				ShowWindow(hwndElevateButton, SW_SHOW);
			}
		}
		return 0;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case BUTTON_ID_1:
			if (LOWORD(wParam) == BUTTON_ID_1 && HIWORD(wParam) == BN_CLICKED)
			{
				// 创建提权对象并执行提权操作
				PrivilegeElevate privilegeElevate;
				if (privilegeElevate.AdmintoSystem())
				{
					exit(0);
				}
				else
				{
					MessageBox(hwnd, L"提权失败！", L"错误", MB_ICONERROR);
				}
				return 0;
			}
			break;
		case ID_MENU_REFRESH:
			// 刷新列表
			processManage.RefreshProcessList();
			FillProcessListView(hwndListView, processManage);
			// 更新状态栏
			statusText = L"进程数量：" + std::to_wstring(processManage.GetProcessCount())
				+ L"    检测到隐藏进程：" + std::to_wstring(processManage.GetProcessHiddenCount());
			SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)statusText.c_str());
			break;
		case ID_MENU_TERMINATE_PROCESS:
		{
			// 获取选中项
			int selectedItem = ListView_GetNextItem(hwndListView, -1, LVNI_SELECTED);
			if (selectedItem != -1)
			{
				LVITEM lvi = { 0 };
				lvi.mask = LVIF_PARAM;
				lvi.iItem = selectedItem;
				ListView_GetItem(hwndListView, &lvi);
				DWORD selectedPid = (DWORD)lvi.lParam;

				// 确认对话框
				if (MessageBox(hwnd,
					L"确定要结束此进程吗？",
					L"确认",
					MB_YESNO | MB_ICONQUESTION) == IDYES)
				{
					// 打开进程并终止
					HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, selectedPid);
					if (hProcess)
					{
						if (TerminateProcess(hProcess, 0))
						{
							// 刷新列表
							processManage.RefreshProcessList();
							FillProcessListView(hwndListView, processManage);
						}
						else
						{
							MessageBox(hwnd, L"无法结束进程！", L"错误", MB_ICONERROR);
						}
						CloseHandle(hProcess);
					}
				}
			}
			break;
		}
		}
	case ID_MENU_PROPERTY:
	{
		// 获取选中项
		int selectedItem = ListView_GetNextItem(hwndListView, -1, LVNI_SELECTED);
		if (selectedItem != -1)
		{
			LVITEM lvi = { 0 };
			lvi.mask = LVIF_PARAM;
			lvi.iItem = selectedItem;
			ListView_GetItem(hwndListView, &lvi);
			DWORD selectedPid = (DWORD)lvi.lParam;

			// 构建命令行
			WCHAR cmdLine[MAX_PATH];
			wsprintfW(cmdLine, L"taskmgr.exe /pid %d", selectedPid);

			// 启动任务管理器并显示属性
			STARTUPINFO si = { sizeof(si) };
			PROCESS_INFORMATION pi;
			if (CreateProcess(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
			{
				CloseHandle(pi.hThread);
				CloseHandle(pi.hProcess);
			}
		}
		break;
	}


	case WM_CONTEXTMENU:
	{
		if ((HWND)wParam == hwndListView)  // 判断是否是列表视图的右键菜单
		{
			// 获取选中项
			int selectedItem = ListView_GetNextItem(hwndListView, -1, LVNI_SELECTED);
			if (selectedItem != -1)
			{
				// 获取选中项的PID
				LVITEM lvi = { 0 };
				lvi.mask = LVIF_PARAM;
				lvi.iItem = selectedItem;
				ListView_GetItem(hwndListView, &lvi);
				DWORD selectedPid = (DWORD)lvi.lParam;

				// 创建弹出菜单
				HMENU hPopupMenu = CreatePopupMenu();
				if (hPopupMenu)
				{
					// 添加菜单项
					AppendMenu(hPopupMenu, MF_STRING, ID_MENU_REFRESH, L"刷新");
					AppendMenu(hPopupMenu, MF_SEPARATOR, 0, NULL);  // 分隔线
					AppendMenu(hPopupMenu, MF_STRING, ID_MENU_TERMINATE_PROCESS, L"结束进程");
					AppendMenu(hPopupMenu, MF_STRING, ID_MENU_TERMINATE_PROCESSTREE, L"结束进程树");
					AppendMenu(hPopupMenu, MF_SEPARATOR, 0, NULL);  // 分隔线
					AppendMenu(hPopupMenu, MF_STRING, ID_MENU_VIEW_PROCESS_MODULES, L"查看进程模块");
					AppendMenu(hPopupMenu, MF_STRING, ID_MENU_PROPERTY, L"属性");
					// 显示菜单
					TrackPopupMenu(hPopupMenu,
						TPM_LEFTALIGN | TPM_RIGHTBUTTON,
						LOWORD(lParam),  // x坐标
						HIWORD(lParam),  // y坐标
						0,
						hwnd,
						NULL);

					DestroyMenu(hPopupMenu);
				}
			}
			return 0;
		}
		break;
	}
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
