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
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);  // ʹ��һ����ɫ�Ļ�ˢ��������ɫ
    wc.lpszClassName = L"Phantom_ARK_Module";  // ʹ�ò�ͬ�����������ͻ

    RegisterClassExW(&wc);

    // �洢����Ĳ������Ա���WM_CREATE��ʹ��
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
        L"ģ����Ϣ - Phantom_ARK v1.0.0",    // Window text
        WS_OVERLAPPEDWINDOW,            // Window style 
        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, 1200 * GetDPI() / 100 * 0.75, 750 * GetDPI() / 100 * 0.75, // ���ó�ʼ��С

        NULL,       // �����ø�����
        NULL,       // Menu
        wc.hInstance,     // Instance handle
        params            // ���ݲ���
    );

    if (hwnd == NULL)
    {
        delete params;
        return 0;
    }

    // ��ȡ��Ļ�ֱ���
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // �������λ��
    int x = (screenWidth - 1200 * GetDPI() / 100 * 0.75) / 2;
    int y = (screenHeight - 750 * GetDPI() / 100 * 0.75) / 2;

    // ���ô��ھ���
    SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);


    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    // ������������Ϣѭ��
    MSG msg = { };
    BOOL bRet;
    while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0)
    {
        if (bRet == -1) {
            // �������
            return 0;
        }
        else if (!IsDialogMessage(hwnd, &msg)) // ����Ի�����̵���
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // ��������ѱ����٣��˳���Ϣѭ��
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

    static int statwidths[] = { 800, -1 };  // -1 ��ʾ���쵽�����ұ�
    static std::wstring statusText;

    switch (uMsg)
    {
    case WM_CREATE:
    {
        // ��ȡ����Ĳ���
        CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
        if (cs->lpCreateParams) {
            struct ModuleWindowParams {
                DWORD processId;
                ProcessManage::ProcessInfo processInfo;
            };

            ModuleWindowParams* params = (ModuleWindowParams*)cs->lpCreateParams;
            currentProcessId = params->processId;
			processInfo = params->processInfo; // ���Ʋ�������Ա����

            // ʹ�ò���������ɾ��
            delete params;
        }

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

        // �����б���ͼ
        hwndListView = CreateWindowEx(
            WS_EX_CLIENTEDGE,
            WC_LISTVIEW,
            NULL,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LVS_REPORT,
            10, // ��߾�
            10, // �����߾�
            rcClient.right - 20, // ���
            rcClient.bottom - 20, // �߶�
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
        lvColumn.pszText = L"ģ������";
        ListView_InsertColumn(hwndListView, 0, &lvColumn);

        lvColumn.cx = 150;
        lvColumn.fmt = LVCFMT_LEFT;
        lvColumn.pszText = L"ģ���ַ";
        ListView_InsertColumn(hwndListView, 1, &lvColumn);

        lvColumn.cx = 150;
        lvColumn.fmt = LVCFMT_LEFT;
        lvColumn.pszText = L"ģ���С";
        ListView_InsertColumn(hwndListView, 2, &lvColumn);

        lvColumn.cx = 600;
        lvColumn.fmt = LVCFMT_LEFT;
        lvColumn.pszText = L"ģ��·��";
        ListView_InsertColumn(hwndListView, 3, &lvColumn);

        lvColumn.cx = 200;
        lvColumn.fmt = LVCFMT_LEFT;
        lvColumn.pszText = L"�ļ�����";
        ListView_InsertColumn(hwndListView, 4, &lvColumn);

        lvColumn.cx = 300;
        lvColumn.fmt = LVCFMT_LEFT;
        lvColumn.pszText = L"�ļ�����";
        ListView_InsertColumn(hwndListView, 5, &lvColumn);

        // ��������Լ��غ���䵱ǰ����ID��ģ���б�
        // FillModuleListView(hwndListView, currentProcessId);

        // ���ô��ڱ�����ʾ��ǰ����ID
        std::wstring windowTitle = L"ģ����Ϣ" + processInfo.getProcessName() 
            + L"[" + std::to_wstring(processInfo.getPid()) + L"] - Phantom_ARK";
        SetWindowText(hwnd, windowTitle.c_str());

        return 0;
    }

    case WM_PAINT:
        // Paint the window's client area. 
        return DefWindowProc(hwnd, uMsg, wParam, lParam);

    case WM_SIZE:
        // �����б���ͼ��С����Ӧ����
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
        // �����б���ͼ��֪ͨ��Ϣ
        return 0;

    case WM_COMMAND:
        // ����������Ϣ
        return 0;

    case WM_CONTEXTMENU:
        // ����������ģ���б���Ҽ��˵�
        return 0;


    case WM_DESTROY:
        // ����������Դ
        if (hFont)
        {
            DeleteObject(hFont);
        }
        // ������PostQuitMessage(0)����Ϊ����ֻ��ر�������ڣ���������Ӧ�ó���
        return 0;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}
