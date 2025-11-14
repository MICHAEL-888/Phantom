#pragma once
#include <windows.h>
#include <string>

class SettingsPage {
public:
    SettingsPage(HINSTANCE hInstance, HWND hParent);
    ~SettingsPage();

    void Move(const RECT& rc);
    void Show(bool show);
    void OnDpiChanged(UINT dpi);
    bool HandleCommand(WORD id, WORD code);

private:
    void CreateControls();
    static LRESULT CALLBACK ContainerWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    HINSTANCE m_hInstance{};
    HWND m_hParent{};
    HWND m_hwnd{}; // container
    HWND m_hwndElevate{};
    HFONT m_hFont{};
    UINT m_dpi{96};
    WNDPROC m_prevProc{};
};
