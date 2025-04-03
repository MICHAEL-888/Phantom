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

void FillProcessListView(HWND hwndListView, const ProcessManage& processManager) {
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
            LVS_EX_LABELTIP);

        // �����б���
        LVCOLUMN lvColumn;
        lvColumn.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        lvColumn.cx = 300;
        lvColumn.pszText = L"��������";
        ListView_InsertColumn(hwndListView, 0, &lvColumn);

        lvColumn.cx = 150;
        lvColumn.pszText = L"����ID";
        ListView_InsertColumn(hwndListView, 1, &lvColumn);

        lvColumn.cx = 600;
        lvColumn.pszText = L"����·��";
        ListView_InsertColumn(hwndListView, 2, &lvColumn);

        
        FillProcessListView(hwndListView, processManage);



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
                rcClient.bottom - tabHeight- rcStatus.bottom + rcStatus.top - 20,  // �߶ȣ������ײ����
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
