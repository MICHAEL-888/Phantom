#pragma once
#include <windows.h>
#include <winuser.h>
#include "ProcessManage.h"
#include <CommCtrl.h>

#define LISTVIEW_ID_1 3000

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

int ModuleWindow(HWND parentHwnd, DWORD processId, std::vector<ProcessManage::ProcessInfo> m_processList) {
	WNDCLASSEXW wc = { };
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_SAVEBITS | CS_DROPSHADOW | CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = ModuleWndProc;
	wc.hInstance = GetModuleHandle(NULL);
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


		return 0;

	case WM_NOTIFY:



		return 0;
	case WM_COMMAND:

	case WM_CONTEXTMENU:
	
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