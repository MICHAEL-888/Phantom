#pragma once
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <memory>
#include "ProcessManage.h"

#define STATUSBAR_ID_1 1000
#define TABCONTROL_ID_1 2000
#define BUTTON_ID_1 4000
#define ID_MENU_REFRESH    5001
#define ID_MENU_TERMINATE_PROCESS  5002
#define ID_MENU_PROPERTY   5003
#define ID_MENU_TERMINATE_PROCESSTREE 5004
#define ID_MENU_VIEW_PROCESS_MODULES 5005

// App-wide custom message to refresh status bar
#ifndef WM_APP_UPDATE_STATUS
#define WM_APP_UPDATE_STATUS (WM_APP + 1)
#endif

class ProcessPage;
class SettingsPage;

class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    bool Register(HINSTANCE hInstance);
    bool Create(HINSTANCE hInstance, int nCmdShow);

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    void InitCommonControlsOnce();
    void InitControls();
    void UpdateStatusBar();
    void ApplyDpiAwareness();
    void RecreateFontForDpi(UINT dpi);
    void LayoutForSize();

private:
    HINSTANCE m_hInstance{};
    HWND m_hwnd{};
    HWND m_hwndStatus{};
    HWND m_hwndTab{};
    HFONT m_hFont{};

    ProcessManage m_processManage{};

    int m_statwidths[2]{800, -1};
    std::wstring m_statusText{};

    std::unique_ptr<ProcessPage> m_processPage;
    std::unique_ptr<SettingsPage> m_settingsPage;
};
