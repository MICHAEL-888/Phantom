#include "MainWindow.h"
#include "ListViewEx.h"
#include "ProcessPage.h"
#include "SettingsPage.h"
#include <shellscalingapi.h>
#include <strsafe.h>

#pragma comment(lib, "Comctl32.lib")

// Provide GetDPI implementation reused by initial sizing code
int GetDPI() {
    HDC hDC = GetDC(NULL);
    int DPI_A = GetDeviceCaps(hDC, 118) / GetDeviceCaps(hDC, 8) * 100;
    int DPI_B = GetDeviceCaps(hDC, 88) / 96 * 100;
    ReleaseDC(NULL, hDC);
    if (DPI_A == 100) return DPI_B;
    else if (DPI_B == 100) return DPI_A;
    else if (DPI_A == DPI_B) return DPI_A;
    else return 100; // fallback
}

MainWindow::MainWindow() {}

MainWindow::~MainWindow() {
    if (m_hFont) DeleteObject(m_hFont);
}

bool MainWindow::Register(HINSTANCE hInstance) {
    m_hInstance = hInstance;
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_SAVEBITS | CS_DROPSHADOW | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MainWindow::WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"Phantom_ARK";
    return RegisterClassExW(&wc) != 0;
}

bool MainWindow::Create(HINSTANCE hInstance, int nCmdShow) {
    ApplyDpiAwareness();
    InitCommonControlsOnce();

    m_hwnd = CreateWindowExW(0, L"Phantom_ARK", L"Phantom_ARK v1.0.0", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1000 * GetDPI() / 100, 625 * GetDPI() / 100,
        nullptr, nullptr, hInstance, this);
    if (!m_hwnd) return false;

    // center
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenWidth - 1000 * GetDPI() / 100) / 2;
    int y = (screenHeight - 625 * GetDPI() / 100) / 2;
    SetWindowPos(m_hwnd, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

    ShowWindow(m_hwnd, nCmdShow);
    UpdateWindow(m_hwnd);
    return true;
}

void MainWindow::InitCommonControlsOnce() {
    INITCOMMONCONTROLSEX icc{ sizeof(icc) };
    icc.dwICC = ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icc);
}

void MainWindow::ApplyDpiAwareness() {
    // Per-Monitor v2 where available
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
}

void MainWindow::RecreateFontForDpi(UINT dpi) {
    if (m_hFont) { DeleteObject(m_hFont); m_hFont = nullptr; }
    m_hFont = CreateFontW(19 * dpi / 100, 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei");
}

void MainWindow::InitControls() {
    UINT dpi = GetDpiForWindow(m_hwnd);
    RecreateFontForDpi(dpi);

    m_hwndStatus = CreateWindowEx(0, STATUSCLASSNAME, NULL, WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0, 0, 0, 0, m_hwnd, (HMENU)STATUSBAR_ID_1, m_hInstance, NULL);

    SendMessage(m_hwndStatus, SB_SETPARTS, 2, (LPARAM)m_statwidths);

    m_hwndTab = CreateWindowEx(0, WC_TABCONTROL, L"", WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE, 0, 0, 300, 200,
        m_hwnd, (HMENU)TABCONTROL_ID_1, m_hInstance, NULL);

    SendMessage(m_hwndTab, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    TCITEM tie{}; tie.mask = TCIF_TEXT; tie.pszText = (LPWSTR)L"进程管理"; TabCtrl_InsertItem(m_hwndTab, 0, &tie);
    tie.pszText = (LPWSTR)L"设置"; TabCtrl_InsertItem(m_hwndTab, 1, &tie);

    // Create pages
    m_processPage = std::make_unique<ProcessPage>(m_hInstance, m_hwnd, m_processManage);
    m_settingsPage = std::make_unique<SettingsPage>(m_hInstance, m_hwnd);

    LayoutForSize();
    UpdateStatusBar();
}

void MainWindow::LayoutForSize() {
    RECT rcClient{}; GetClientRect(m_hwnd, &rcClient);

    SendMessage(m_hwndStatus, WM_SIZE, 0, 0);

    SetWindowPos(m_hwndTab, NULL, 0, 0, rcClient.right, rcClient.bottom, SWP_NOZORDER);

    RECT rcTabItem{}; TabCtrl_GetItemRect(m_hwndTab, 0, &rcTabItem);
    int tabHeight = rcTabItem.bottom + 4;

    RECT rcStatus{}; GetWindowRect(m_hwndStatus, &rcStatus);
    int statusHeight = rcStatus.bottom - rcStatus.top;

    RECT pageRect{ 10, tabHeight + 10, rcClient.right - 10, rcClient.bottom - statusHeight - 10 };

    if (m_processPage) m_processPage->Move(pageRect);
    if (m_settingsPage) m_settingsPage->Move(pageRect);

    int page = TabCtrl_GetCurSel(m_hwndTab);
    if (m_processPage) m_processPage->Show(page == 0);
    if (m_settingsPage) m_settingsPage->Show(page == 1);
}

void MainWindow::UpdateStatusBar() {
    std::wstring statusText = L"进程数量：" + std::to_wstring(m_processManage.GetProcessCount()) +
        L"    检测到隐藏进程：" + std::to_wstring(m_processManage.GetProcessHiddenCount());
    SendMessage(m_hwndStatus, SB_SETTEXT, 0, (LPARAM)statusText.c_str());
    SendMessage(m_hwndStatus, SB_SETTEXT, 1, (LPARAM)L"Phantom_ARK v1.0.0");
}

LRESULT CALLBACK MainWindow::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    MainWindow* self = nullptr;
    if (uMsg == WM_NCCREATE) {
        auto cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        self = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
        self->m_hwnd = hwnd;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)self);
    } else {
        self = reinterpret_cast<MainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (self) return self->HandleMessage(uMsg, wParam, lParam);
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        InitControls();
        return 0;
    case WM_SIZE:
        LayoutForSize();
        return 0;
    case WM_NOTIFY: {
        LPNMHDR hdr = (LPNMHDR)lParam;
        if (hdr->idFrom == TABCONTROL_ID_1 && hdr->code == TCN_SELCHANGE) {
            LayoutForSize();
            return 0;
        }
        return 0;
    }
    case WM_DPICHANGED: {
        UINT dpiX = HIWORD(wParam);
        RecreateFontForDpi(dpiX);
        if (m_hwndTab) SendMessage(m_hwndTab, WM_SETFONT, (WPARAM)m_hFont, TRUE);
        if (m_processPage) m_processPage->OnDpiChanged(dpiX);
        if (m_settingsPage) m_settingsPage->OnDpiChanged(dpiX);
        RECT* prcNew = (RECT*)lParam;
        SetWindowPos(m_hwnd, NULL, prcNew->left, prcNew->top, prcNew->right - prcNew->left, prcNew->bottom - prcNew->top, SWP_NOZORDER | SWP_NOACTIVATE);
        LayoutForSize();
        UpdateStatusBar();
        return 0;
    }
    case WM_APP_UPDATE_STATUS:
        UpdateStatusBar();
        return 0;
    case WM_COMMAND: {
        if (m_processPage && m_processPage->HandleCommand(LOWORD(wParam), HIWORD(wParam))) { UpdateStatusBar(); return 0; }
        if (m_settingsPage && m_settingsPage->HandleCommand(LOWORD(wParam), HIWORD(wParam))) { UpdateStatusBar(); return 0; }
        return 0;
    }
    case WM_CONTEXTMENU: {
        // allow page to show menu; status update may change after actions
        if (m_processPage && m_processPage->HandleContextMenu((HWND)wParam, LOWORD(lParam), HIWORD(lParam))) { UpdateStatusBar(); return 0; }
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
    }
}
