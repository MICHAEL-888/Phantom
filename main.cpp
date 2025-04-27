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
	// �˺����ο�����ģ���ȡϵͳDPI
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
	int column = LOWORD(lParamSort);    // �к�
	bool ascending = HIWORD(lParamSort); // ������

	HWND hList = GetDlgItem(GetActiveWindow(), LISTVIEW_ID_1);
	WCHAR szText1[256]{};
	WCHAR szText2[256]{};

	// ��ȡҪ�Ƚϵ���������ı�
	int index1 = -1, index2 = -1;

	// �ҵ���ӦPID���������
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

	// ��ȡ�ı����бȽ�
	ListView_GetItemText(hList, index1, column, szText1, _countof(szText1));
	ListView_GetItemText(hList, index2, column, szText2, _countof(szText2));

	ULONG result;
	// ���ݲ�ͬ��ʹ�ò�ͬ�ıȽϷ���
	switch (column)
	{
	case 1: // PID�� - ֱ�ӱȽ�lParam
		result = (ULONG)(lParam1 - lParam2);
		break;
	case 4: // ������ID�� - ���ֱȽ�
	{
		ULONG num1 = _wtoi(szText1);
		ULONG num2 = _wtoi(szText2);
		result = num1 - num2;
		break;
	}
	default: // ������ - �ַ����Ƚ�
		result = _wcsicmp(szText1, szText2);
	}

	return ascending ? result : -result;
}





void FillProcessListView(HWND hwndListView, const ProcessManage& processManager) {
	// �����ػ�
	SendMessage(hwndListView, WM_SETREDRAW, FALSE, 0);

	// ����б���ͼ
	ListView_DeleteAllItems(hwndListView);

	// ��ȡ�����б�
	const auto& processList = processManager.GetProcessList();

	// ���ÿ�����̵���Ϣ
	for (size_t i = 0; i < processList.size(); i++) {
		// �����б���
		LVITEM lvItem;
		ZeroMemory(&lvItem, sizeof(lvItem));

		// ���õ�һ�У��������ƣ�
		lvItem.mask = LVIF_TEXT | LVIF_PARAM;
		lvItem.iItem = static_cast<int>(i);
		lvItem.iSubItem = 0;
		lvItem.pszText = const_cast<LPWSTR>(processList[i].m_processName.c_str());
		lvItem.lParam = processList[i].m_pid; // �洢PID���Ա��Ժ�����

		// �����б���
		int index = ListView_InsertItem(hwndListView, &lvItem);

		// ���õڶ��У�����ID��
		std::wstring pidStr = std::to_wstring(processList[i].m_pid);
		ListView_SetItemText(hwndListView, index, 1, const_cast<LPWSTR>(pidStr.c_str()));

		// ���õ����У�����·����
		ListView_SetItemText(hwndListView, index, 2, const_cast<LPWSTR>(processList[i].m_processPath.c_str()));

		// ���õ����У����ؽ��̼�⣩
		std::wstring isHideStr = processList[i].m_isHide ? L"true" : L"";
		ListView_SetItemText(hwndListView, index, 3, const_cast<LPWSTR>(isHideStr.c_str()));

		// ���õ����У�������ID��
		std::wstring parrentPidStr = std::to_wstring(processList[i].m_parrentProcessId);
		ListView_SetItemText(hwndListView, index, 4, const_cast<LPWSTR>(parrentPidStr.c_str()));

		// ���õ����У�����������
		ListView_SetItemText(hwndListView, index, 5, const_cast<LPWSTR>(processList[i].m_parrentProcessName.c_str()));

		// ���õ����У��ؼ����̣�
		std::wstring isCriticalStr = processList[i].m_isCritical ? L"true" : L"";
		ListView_SetItemText(hwndListView, index, 6, const_cast<LPWSTR>(isCriticalStr.c_str()));

		// ���õڰ��У�PPL��
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

		// ���õھ��У��û�����
		if (!processList[i].m_userDomain.empty() && !processList[i].m_userName.empty()) {
			std::wstring str9 = processList[i].m_userDomain + L"\\" + processList[i].m_userName;
			ListView_SetItemText(hwndListView, index, 8, const_cast<LPWSTR>(str9.c_str()));
		}
		
		// ���õ�ʮ�У�PEB��ַ��
		if (processList[i].m_peb != nullptr) {
			std::wstringstream wss10;
			wss10 << L"0x" << std::uppercase << std::hex 
				<< reinterpret_cast<uintptr_t>(processList[i].m_peb);
			std::wstring str10 = wss10.str();
			ListView_SetItemText(hwndListView, index, 9, const_cast<LPWSTR>(str10.c_str()));
		}
		
		

	}

	// �����ػ�
	SendMessage(hwndListView, WM_SETREDRAW, TRUE, 0);

	// ǿ���ػ��б���ͼ�ͱ�ͷ
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


	static int statwidths[] = { 800, -1 };  // -1 ��ʾ���쵽�����ұ�
	static std::wstring statusText;

	switch (uMsg)
	{
	case WM_CREATE:
		// ��������
		hFont = CreateFontW(
			19 * GetDPI() / 100,       // ����߶�
			0,                         // ������
			0,                         // ���ֽǶ�
			0,                         // ���߽Ƕ�
			FW_NORMAL,                 // �����ϸ
			FALSE,                     // б��
			FALSE,                     // �»���
			FALSE,                     // ɾ����
			DEFAULT_CHARSET,           // �ַ���
			OUT_TT_ONLY_PRECIS,        // �������
			CLIP_DEFAULT_PRECIS,       // �ü�����
			CLEARTYPE_QUALITY,         // �������
			DEFAULT_PITCH | FF_DONTCARE,  // ���������
			L"Segoe UI Variable Display");           // ��������

		// ״̬�� 
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

		// ����״̬������

		SendMessage(hwndStatus, SB_SETPARTS, 2, (LPARAM)statwidths);

		// ���ó�ʼ״̬���ı�

		statusText = L"����������" + std::to_wstring(processManage.GetProcessCount())
			+ L"    ��⵽���ؽ��̣�" + std::to_wstring(processManage.GetProcessHiddenCount());
		SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)statusText.c_str());
		SendMessage(hwndStatus, SB_SETTEXT, 1, (LPARAM)L"Phantom_ARK v1.0.0");


		// ѡ�
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

		// Tab��������
		SendMessage(hwndTab, WM_SETFONT, (WPARAM)hFont, TRUE);

		TCITEM tie;
		tie.mask = TCIF_TEXT;
		tie.pszText = L"���̹���";
		TabCtrl_InsertItem(hwndTab, 0, &tie);

		tie.pszText = L"����";
		TabCtrl_InsertItem(hwndTab, 1, &tie);

		// ��ȡ�ͻ�������
		RECT rcClient;
		GetClientRect(hwnd, &rcClient);

		// ��ȡѡ��ؼ�������
		RECT rcTab;
		GetClientRect(hwndTab, &rcTab);

		// �����б���λ�ã�ʹ��λ��ѡ��Ŀͻ�����
		RECT rcTabClient;
		TabCtrl_GetItemRect(hwndTab, 0, &rcTabClient);

		// ������һ��ѡ���Ӧ���б��
		hwndListView = CreateWindowEx(
			WS_EX_CLIENTEDGE,
			WC_LISTVIEW,
			NULL,
			WS_CHILD | WS_VISIBLE | WS_VSCROLL | LVS_REPORT,
			10, // ��߾�
			rcTabClient.bottom + 4 + 10, // �����߾�
			rcClient.right - 20, // ���
			rcClient.bottom - rcTabClient.bottom + 4 - 20, // �߶�
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

		// �����б���
		LVCOLUMN lvColumn;
		lvColumn.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;
		lvColumn.cx = 300;
		lvColumn.fmt = LVCFMT_LEFT;
		lvColumn.pszText = L"��������";
		ListView_InsertColumn(hwndListView, 0, &lvColumn);

		lvColumn.cx = 150;
		lvColumn.fmt = LVCFMT_LEFT;
		lvColumn.pszText = L"����ID";
		ListView_InsertColumn(hwndListView, 1, &lvColumn);

		lvColumn.cx = 600;
		lvColumn.fmt = LVCFMT_LEFT;
		lvColumn.pszText = L"ӳ��·��";
		ListView_InsertColumn(hwndListView, 2, &lvColumn);

		lvColumn.cx = 180;
		lvColumn.fmt = LVCFMT_CENTER;  // ���þ��ж���
		lvColumn.pszText = L"���ؽ��̼��";
		ListView_InsertColumn(hwndListView, 3, &lvColumn);

		lvColumn.cx = 150;
		lvColumn.fmt = LVCFMT_LEFT;
		lvColumn.pszText = L"������ID";
		ListView_InsertColumn(hwndListView, 4, &lvColumn);

		lvColumn.cx = 300;
		lvColumn.fmt = LVCFMT_LEFT;
		lvColumn.pszText = L"��������";
		ListView_InsertColumn(hwndListView, 5, &lvColumn);

		lvColumn.cx = 180;
		lvColumn.fmt = LVCFMT_CENTER;  // ���þ��ж���
		lvColumn.pszText = L"ϵͳ�ؼ�����";
		ListView_InsertColumn(hwndListView, 6, &lvColumn);

		lvColumn.cx = 300;
		lvColumn.fmt = LVCFMT_LEFT;
		lvColumn.pszText = L"���̱���";
		ListView_InsertColumn(hwndListView, 7, &lvColumn);

		lvColumn.cx = 350;
		lvColumn.fmt = LVCFMT_LEFT;
		lvColumn.pszText = L"�û���";
		ListView_InsertColumn(hwndListView, 8, &lvColumn);

		lvColumn.cx = 300;
		lvColumn.fmt = LVCFMT_LEFT;
		lvColumn.pszText = L"PEB��ַ";
		ListView_InsertColumn(hwndListView, 9, &lvColumn);
		
		FillProcessListView(hwndListView, processManage);

		hwndElevateButton = CreateWindowEx(
			0,
			L"BUTTON",
			L"������SYSTEMȨ��",
			WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			rcTabClient.right - 100,  // xλ��
			rcTabClient.bottom - 4 - 10,  // yλ��
			200 * GetDPI() / 100,  // ���
			40 * GetDPI() / 100,   // �߶�
			hwnd,
			(HMENU)BUTTON_ID_1,
			GetModuleHandle(NULL),
			NULL
		);

		// ���ð�ť����
		SendMessage(hwndElevateButton, WM_SETFONT, (WPARAM)hFont, TRUE);

		// ��ʼ���ذ�ť����ΪĬ����ʾ��һ��ѡ���
		ShowWindow(hwndElevateButton, SW_HIDE);

		return 0;

	case WM_PAINT:
		// Paint the window's client area. 
		return DefWindowProc(hwnd, uMsg, wParam, lParam);

	case WM_SIZE:
		// Set the size and position of the window. 
		SendMessage(hwndStatus, WM_SIZE, 0, 0); // ʹ�ؼ��ɱ��С

		if (hwndTab)
		{
			RECT rcClient;  // ���ڿͻ������꣬���Ϻ�����
			GetClientRect(hwnd, &rcClient);
			SetWindowPos(hwndTab, NULL, 0, 0, rcClient.right, rcClient.bottom, SWP_NOZORDER);
		}

		// �����б���ͼ��λ��
		if (hwndTab && hwndListView)
		{
			RECT rcClient;  // ���ڿͻ������꣬���Ϻ�����
			GetClientRect(hwnd, &rcClient);
			// ��ȡѡ��ĳߴ���Ϣ
			RECT rcTabClient;
			TabCtrl_GetItemRect(hwndTab, 0, &rcTabClient);
			int tabHeight = rcTabClient.bottom + 4; // ѡ�����߶ȼ�һ��߾�

			// ��ȡ״̬���ĸ߶�
			RECT rcStatus;
			GetWindowRect(hwndStatus, &rcStatus);
			int statusHeight = rcStatus.bottom - rcStatus.top;

			// �����б���ͼ��λ�úʹ�С
			SetWindowPos(hwndListView, NULL,
				10,                       // ��߾�
				tabHeight + 10,           // �����߾�
				rcClient.right - 20,      // ���
				rcClient.bottom - tabHeight - rcStatus.bottom + rcStatus.top - 20,  // �߶ȣ������ײ����
				SWP_NOZORDER);
		}

		// ������Ȩ��ťλ��
		if (hwndTab && hwndElevateButton)
		{
			RECT rcClient;
			GetClientRect(hwnd, &rcClient);
			RECT rcTabClient;
			TabCtrl_GetItemRect(hwndTab, 0, &rcTabClient);
			int tabHeight = rcTabClient.bottom + 4;

			// ��ťλ�õ���
			SetWindowPos(hwndElevateButton, NULL,
				10,                    // xλ��
				tabHeight + 10,        // yλ��
				400,                   // ���
				200,                    // �߶�
				SWP_NOZORDER);
		}

		return 0;

	case WM_NOTIFY:
		if (((LPNMHDR)lParam)->hwndFrom == ListView_GetHeader(hwndListView))
		{
			if (((LPNMHDR)lParam)->code == HDN_ITEMCLICK)
			{
				LPNMHEADER phdr = (LPNMHEADER)lParam;
				// ��ȡ�����������
				int column = phdr->iItem;
				static bool ascending = true;  // ������

				SendMessage(hwndListView, WM_SETREDRAW, FALSE, 0);
				// ���б����������
				ListView_SortItems(hwndListView, CompareFunc, MAKELPARAM(column, ascending));
				ascending = !ascending;  // �л�������
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
				// ������Ȩ����ִ����Ȩ����
				PrivilegeElevate privilegeElevate;
				if (privilegeElevate.AdmintoSystem())
				{
					exit(0);
				}
				else
				{
					MessageBox(hwnd, L"��Ȩʧ�ܣ�", L"����", MB_ICONERROR);
				}
				return 0;
			}
			break;
		case ID_MENU_REFRESH:
			// ˢ���б�
			processManage.RefreshProcessList();
			FillProcessListView(hwndListView, processManage);
			// ����״̬��
			statusText = L"����������" + std::to_wstring(processManage.GetProcessCount())
				+ L"    ��⵽���ؽ��̣�" + std::to_wstring(processManage.GetProcessHiddenCount());
			SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)statusText.c_str());
			break;
		case ID_MENU_TERMINATE_PROCESS:
		{
			// ��ȡѡ����
			int selectedItem = ListView_GetNextItem(hwndListView, -1, LVNI_SELECTED);
			if (selectedItem != -1)
			{
				LVITEM lvi = { 0 };
				lvi.mask = LVIF_PARAM;
				lvi.iItem = selectedItem;
				ListView_GetItem(hwndListView, &lvi);
				DWORD selectedPid = (DWORD)lvi.lParam;

				// ȷ�϶Ի���
				if (MessageBox(hwnd,
					L"ȷ��Ҫ�����˽�����",
					L"ȷ��",
					MB_YESNO | MB_ICONQUESTION) == IDYES)
				{
					// �򿪽��̲���ֹ
					HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, selectedPid);
					if (hProcess)
					{
						if (TerminateProcess(hProcess, 0))
						{
							// ˢ���б�
							processManage.RefreshProcessList();
							FillProcessListView(hwndListView, processManage);
						}
						else
						{
							MessageBox(hwnd, L"�޷��������̣�", L"����", MB_ICONERROR);
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
		// ��ȡѡ����
		int selectedItem = ListView_GetNextItem(hwndListView, -1, LVNI_SELECTED);
		if (selectedItem != -1)
		{
			LVITEM lvi = { 0 };
			lvi.mask = LVIF_PARAM;
			lvi.iItem = selectedItem;
			ListView_GetItem(hwndListView, &lvi);
			DWORD selectedPid = (DWORD)lvi.lParam;

			// ����������
			WCHAR cmdLine[MAX_PATH];
			wsprintfW(cmdLine, L"taskmgr.exe /pid %d", selectedPid);

			// �����������������ʾ����
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
		if ((HWND)wParam == hwndListView)  // �ж��Ƿ����б���ͼ���Ҽ��˵�
		{
			// ��ȡѡ����
			int selectedItem = ListView_GetNextItem(hwndListView, -1, LVNI_SELECTED);
			if (selectedItem != -1)
			{
				// ��ȡѡ�����PID
				LVITEM lvi = { 0 };
				lvi.mask = LVIF_PARAM;
				lvi.iItem = selectedItem;
				ListView_GetItem(hwndListView, &lvi);
				DWORD selectedPid = (DWORD)lvi.lParam;

				// ���������˵�
				HMENU hPopupMenu = CreatePopupMenu();
				if (hPopupMenu)
				{
					// ��Ӳ˵���
					AppendMenu(hPopupMenu, MF_STRING, ID_MENU_REFRESH, L"ˢ��");
					AppendMenu(hPopupMenu, MF_SEPARATOR, 0, NULL);  // �ָ���
					AppendMenu(hPopupMenu, MF_STRING, ID_MENU_TERMINATE_PROCESS, L"��������");
					AppendMenu(hPopupMenu, MF_STRING, ID_MENU_TERMINATE_PROCESSTREE, L"����������");
					AppendMenu(hPopupMenu, MF_SEPARATOR, 0, NULL);  // �ָ���
					AppendMenu(hPopupMenu, MF_STRING, ID_MENU_VIEW_PROCESS_MODULES, L"�鿴����ģ��");
					AppendMenu(hPopupMenu, MF_STRING, ID_MENU_PROPERTY, L"����");
					// ��ʾ�˵�
					TrackPopupMenu(hPopupMenu,
						TPM_LEFTALIGN | TPM_RIGHTBUTTON,
						LOWORD(lParam),  // x����
						HIWORD(lParam),  // y����
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
		// ����������Դ
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
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);  // ʹ��һ����ɫ�Ļ�ˢ��������ɫ
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
