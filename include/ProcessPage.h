#pragma once
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include "ProcessManage.h"

class ProcessPage {
public:
    ProcessPage(HINSTANCE hInstance, HWND hParent, ProcessManage& pm);
    ~ProcessPage();

    void Move(const RECT& rc);
    void Show(bool show);
    void OnDpiChanged(UINT dpi);

    bool HandleCommand(WORD id, WORD code);
    bool HandleContextMenu(HWND hwndFrom, int x, int y);

    struct SortContext {
        int column{};
        bool ascending{};
        HWND hwndList{};
    };

private:
    void CreateControls();
    void FillProcessList();
    void UpdateStatus();
    static LRESULT CALLBACK ContainerWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    HINSTANCE m_hInstance{};
    HWND m_hParent{};
    HWND m_hwnd{};           // container window for the page
    HWND m_hwndList{};
    HFONT m_hFont{};

    ProcessManage& m_pm;
    std::vector<bool> m_sortAscending;
    SortContext m_sortCtx{};
    UINT m_dpi{96};
    WNDPROC m_prevProc{};
};
