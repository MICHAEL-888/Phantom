#include "SettingsPage.h"
#include <commctrl.h>
#include "PrivilegeElevate.h"

#define BUTTON_ID_1 4000

SettingsPage::SettingsPage(HINSTANCE hInstance, HWND hParent)
    : m_hInstance(hInstance), m_hParent(hParent) {
    m_dpi = GetDpiForWindow(hParent);
    m_hwnd = CreateWindowEx(0, L"STATIC", NULL, WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0, m_hParent, NULL, m_hInstance, NULL);
    // Subclass container to catch WM_COMMAND routed via parent
    m_prevProc = (WNDPROC)SetWindowLongPtr(m_hwnd, GWLP_WNDPROC, (LONG_PTR)ContainerWndProc);
    SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
    CreateControls();
}

SettingsPage::~SettingsPage() {
    if (m_hFont) DeleteObject(m_hFont);
}

void SettingsPage::CreateControls() {
    if (m_hFont) { DeleteObject(m_hFont); m_hFont = nullptr; }
    m_hFont = CreateFontW(17 * m_dpi / 100, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei");

    if (!m_hwndElevate) {
        m_hwndElevate = CreateWindowEx(0, L"BUTTON", L"提升至SYSTEM权限", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            10, 10, 200 * m_dpi / 100, 100 * m_dpi / 100, m_hwnd, (HMENU)BUTTON_ID_1, m_hInstance, NULL);
    }
    SendMessage(m_hwndElevate, WM_SETFONT, (WPARAM)m_hFont, TRUE);
}

void SettingsPage::Move(const RECT& rc) {
    SetWindowPos(m_hwnd, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER);
    SetWindowPos(m_hwndElevate, NULL, 10, 10, 200 * m_dpi / 100, 100 * m_dpi / 100, SWP_NOZORDER);
}

void SettingsPage::Show(bool show) {
    ShowWindow(m_hwnd, show ? SW_SHOW : SW_HIDE);
}

void SettingsPage::OnDpiChanged(UINT dpi) {
    m_dpi = dpi;
    CreateControls();
}

bool SettingsPage::HandleCommand(WORD id, WORD code) {
    if (id == BUTTON_ID_1 && code == BN_CLICKED) {
        PrivilegeElevate privilegeElevate;
        if (privilegeElevate.AdmintoSystem()) {
            exit(0);
        } else {
            MessageBox(m_hwnd, L"提权失败！", L"错误", MB_ICONERROR);
        }
        return true;
    }
    return false;
}

LRESULT CALLBACK SettingsPage::ContainerWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto self = reinterpret_cast<SettingsPage*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (self) {
        if (msg == WM_COMMAND) {
            WORD id = LOWORD(wParam), code = HIWORD(wParam);
            if (self->HandleCommand(id, code)) return 0;
        }
        return CallWindowProc(self->m_prevProc, hwnd, msg, wParam, lParam);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
