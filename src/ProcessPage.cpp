#include "ProcessPage.h"
#include "ListViewEx.h"
#include "PrivilegeElevate.h"
#include "ModuleWindow.h"
#include "MainWindow.h"
#include <strsafe.h>
#include <windowsx.h>
#include <uxtheme.h>
#pragma comment(lib, "UxTheme.lib")

#define LISTVIEW_ID_1 3000
#define ID_MENU_REFRESH    5001
#define ID_MENU_TERMINATE_PROCESS  5002
#define ID_MENU_PROPERTY   5003
#define ID_MENU_TERMINATE_PROCESSTREE 5004
#define ID_MENU_VIEW_PROCESS_MODULES 5005

static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort) {
    auto ctx = reinterpret_cast<ProcessPage::SortContext*>(lParamSort);
    if (!ctx || !ctx->hwndList) return 0;
    int column = ctx->column;
    bool ascending = ctx->ascending;
    HWND hList = ctx->hwndList;

    WCHAR buf1[256]{}; WCHAR buf2[256]{};
    int idx1 = -1, idx2 = -1;
    int count = ListView_GetItemCount(hList);
    for (int i = 0; i < count; ++i) {
        LVITEM lvi{}; lvi.mask = LVIF_PARAM; lvi.iItem = i; ListView_GetItem(hList, &lvi);
        if (lvi.lParam == lParam1) idx1 = i;
        if (lvi.lParam == lParam2) idx2 = i;
        if (idx1 != -1 && idx2 != -1) break;
    }
    if (idx1 == -1 || idx2 == -1) return 0;

    ListView_GetItemText(hList, idx1, column, buf1, ARRAYSIZE(buf1));
    ListView_GetItemText(hList, idx2, column, buf2, ARRAYSIZE(buf2));

    int result = 0;
    switch (column) {
    case 1:
    case 4: {
        unsigned long n1 = _wtol(buf1);
        unsigned long n2 = _wtol(buf2);
        result = (n1 < n2) ? -1 : (n1 > n2 ? 1 : 0);
        break;
    }
    default:
        result = _wcsicmp(buf1, buf2);
        break;
    }
    return ascending ? result : -result;
}

ProcessPage::ProcessPage(HINSTANCE hInstance, HWND hParent, ProcessManage& pm)
    : m_hInstance(hInstance), m_hParent(hParent), m_pm(pm) {
    m_dpi = GetDpiForWindow(hParent);
    m_sortAscending.assign(10, true);

    // Container window
    m_hwnd = CreateWindowEx(0, L"STATIC", NULL, WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0, m_hParent, NULL, m_hInstance, NULL);
    // Subclass container to receive WM_NOTIFY/WM_CONTEXTMENU/WM_COMMAND
    m_prevProc = (WNDPROC)SetWindowLongPtr(m_hwnd, GWLP_WNDPROC, (LONG_PTR)ContainerWndProc);
    SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);

    // Create controls
    CreateControls();
}

ProcessPage::~ProcessPage() {
    if (m_hFont) DeleteObject(m_hFont);
}

void ProcessPage::CreateControls() {
    if (m_hFont) { DeleteObject(m_hFont); m_hFont = nullptr; }
    // 使用与 ModuleWindow 类似的字体样式（微软雅黑 / Microsoft YaHei）
    m_hFont = CreateFontW(17 * m_dpi / 100, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei");

    m_hwndList = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, NULL,
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LVS_REPORT,
        0, 0, 0, 0, m_hwnd, (HMENU)LISTVIEW_ID_1, m_hInstance, NULL);

    SetWindowTheme(m_hwndList, L"Explorer", NULL);

    LONG_PTR style = GetWindowLongPtr(m_hwndList, GWL_STYLE);
    style |= LVS_REPORT;
    SetWindowLongPtr(m_hwndList, GWL_STYLE, style);

    DWORD ex = ListView_GetExtendedListViewStyle(m_hwndList);
    ex |= (LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER |
        LVS_EX_HEADERDRAGDROP | LVS_EX_HEADERINALLVIEWS | LVS_EX_LABELTIP);
    ex &= ~LVS_EX_TRANSPARENTBKGND;
    ListView_SetExtendedListViewStyle(m_hwndList, ex);

    ListViewEx lv(m_hwndList);
    lv.SetExtendedStyles(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER |
        LVS_EX_HEADERDRAGDROP | LVS_EX_HEADERINALLVIEWS | LVS_EX_LABELTIP);

    ListView_SetBkColor(m_hwndList, GetSysColor(COLOR_WINDOW));
    ListView_SetTextBkColor(m_hwndList, GetSysColor(COLOR_WINDOW));

    lv.InsertColumn(0, L"进程名称", 150 * m_dpi / 100, LVCFMT_LEFT);
    lv.InsertColumn(1, L"进程ID", 75 * m_dpi / 100, LVCFMT_LEFT);
    lv.InsertColumn(2, L"映像路径", 300 * m_dpi / 100, LVCFMT_LEFT);
    lv.InsertColumn(3, L"隐藏进程检测", 90 * m_dpi / 100, LVCFMT_CENTER);
    lv.InsertColumn(4, L"父进程ID", 75 * m_dpi / 100, LVCFMT_LEFT);
    lv.InsertColumn(5, L"父进程名", 150 * m_dpi / 100, LVCFMT_LEFT);
    lv.InsertColumn(6, L"系统关键进程", 90 * m_dpi / 100, LVCFMT_CENTER);
    lv.InsertColumn(7, L"进程保护", 150 * m_dpi / 100, LVCFMT_LEFT);
    lv.InsertColumn(8, L"用户名", 175 * m_dpi / 100, LVCFMT_LEFT);
    lv.InsertColumn(9, L"PEB基址", 150 * m_dpi / 100, LVCFMT_LEFT);

    SendMessage(m_hwndList, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    m_pm.RefreshProcessList();
    FillProcessList();
}

void ProcessPage::FillProcessList() {
    ListViewEx lv(m_hwndList);
    lv.BeginUpdate();
    lv.Clear();

    const auto& processList = m_pm.GetProcessList();
    for (size_t i = 0; i < processList.size(); ++i) {
        LVITEM item{}; item.mask = LVIF_TEXT | LVIF_PARAM; item.iItem = (int)i; item.iSubItem = 0;
        std::wstring name = processList[i].getProcessName();
        item.pszText = const_cast<LPWSTR>(name.c_str());
        item.lParam = processList[i].getPid();
        int idx = ListView_InsertItem(m_hwndList, &item);

        std::wstring pidStr = std::to_wstring(processList[i].getPid());
        ListView_SetItemText(m_hwndList, idx, 1, const_cast<LPWSTR>(pidStr.c_str()));

        std::wstring path = processList[i].getProcessPath();
        ListView_SetItemText(m_hwndList, idx, 2, const_cast<LPWSTR>(path.c_str()));

        std::wstring hideStr = processList[i].getIsHide() ? L"true" : L"";
        ListView_SetItemText(m_hwndList, idx, 3, const_cast<LPWSTR>(hideStr.c_str()));

        std::wstring ppidStr = std::to_wstring(processList[i].getParrentProcessId());
        ListView_SetItemText(m_hwndList, idx, 4, const_cast<LPWSTR>(ppidStr.c_str()));

        std::wstring pname = processList[i].getParrentProcessName();
        ListView_SetItemText(m_hwndList, idx, 5, const_cast<LPWSTR>(pname.c_str()));

        std::wstring criticalStr = processList[i].getIsCritical() ? L"true" : L"";
        ListView_SetItemText(m_hwndList, idx, 6, const_cast<LPWSTR>(criticalStr.c_str()));

        if ((processList[i].getPpl() & 0b00000111) != 0) {
            std::wstring type{}; std::wstring signer{};
            auto ppl = processList[i].getPpl();
            if ((ppl & 0b00000111) == 1) type = L"Light"; else if ((ppl & 0b00000111) == 2) type = L"Full";
            if ((ppl & 0b11110000) == 0) signer = L"(None)";
            else if ((ppl & 0b11110000) == 16) signer = L"(Authenticode)";
            else if ((ppl & 0b11110000) == 32) signer = L"(CodeGen)";
            else if ((ppl & 0b11110000) == 48) signer = L"(Antimalware)";
            else if ((ppl & 0b11110000) == 64) signer = L"(Lsa)";
            else if ((ppl & 0b11110000) == 80) signer = L"(Windows)";
            else if ((ppl & 0b11110000) == 96) signer = L"(WinTcb)";
            else if ((ppl & 0b11110000) == 112) signer = L"(WinSystem)";
            else if ((ppl & 0b11110000) == 128) signer = L"(App)";
            else if ((ppl & 0b11110000) == 144) signer = L"(Max)";
            std::wstring tmp = type + L" " + signer;
            ListView_SetItemText(m_hwndList, idx, 7, const_cast<LPWSTR>(tmp.c_str()));
        }

        if (!processList[i].getUserDomain().empty() && !processList[i].getUserName().empty()) {
            std::wstring user = processList[i].getUserDomain() + L"\\" + processList[i].getUserName();
            ListView_SetItemText(m_hwndList, idx, 8, const_cast<LPWSTR>(user.c_str()));
        }

        if (processList[i].getPeb() != nullptr) {
            wchar_t buf[64];
            StringCchPrintfW(buf, 64, L"0x%p", processList[i].getPeb());
            ListView_SetItemText(m_hwndList, idx, 9, buf);
        }
    }

    lv.EndUpdate();
}

void ProcessPage::Move(const RECT& rc) {
    SetWindowPos(m_hwnd, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER);
    // layout children: list occupies all
    SetWindowPos(m_hwndList, NULL, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER);
}

void ProcessPage::Show(bool show) {
    ShowWindow(m_hwnd, show ? SW_SHOW : SW_HIDE);
}

void ProcessPage::OnDpiChanged(UINT dpi) {
    m_dpi = dpi;
    CreateControls();
}

bool ProcessPage::HandleCommand(WORD id, WORD code) {
    switch (id) {
    case ID_MENU_REFRESH:
        m_pm.RefreshProcessList();
        FillProcessList();
        UpdateStatus();
        return true;
    case ID_MENU_TERMINATE_PROCESS: {
        int sel = ListView_GetNextItem(m_hwndList, -1, LVNI_SELECTED);
        if (sel != -1) {
            LVITEM lvi{}; lvi.mask = LVIF_PARAM; lvi.iItem = sel; ListView_GetItem(m_hwndList, &lvi);
            DWORD pid = (DWORD)lvi.lParam;
            if (MessageBox(m_hwnd, L"确定要结束此进程吗？", L"确认", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                m_pm.ForceTerminateProcessbyZwApi(pid);
                m_pm.ForceTerminateProcessbyApc(pid);
                UpdateStatus();
            }
        }
        return true;
    }
    case ID_MENU_VIEW_PROCESS_MODULES: {
        int sel = ListView_GetNextItem(m_hwndList, -1, LVNI_SELECTED);
        if (sel != -1) {
            LVITEM lvi{}; lvi.mask = LVIF_PARAM; lvi.iItem = sel; ListView_GetItem(m_hwndList, &lvi);
            DWORD pid = (DWORD)lvi.lParam;
            ProcessManage::ProcessInfo info{};
            for (const auto& p : m_pm.GetProcessList()) { if (p.getPid() == pid) { info = p; break; } }
            ModuleWindow(m_hwnd, pid, info);
        }
        return true;
    }
    }
    return false;
}

bool ProcessPage::HandleContextMenu(HWND hwndFrom, int x, int y) {
    if (hwndFrom == m_hwndList) {
        int sel = ListView_GetNextItem(m_hwndList, -1, LVNI_SELECTED);
        if (sel != -1) {
            HMENU hPopup = CreatePopupMenu();
            if (hPopup) {
                AppendMenu(hPopup, MF_STRING, ID_MENU_REFRESH, L"刷新");
                AppendMenu(hPopup, MF_SEPARATOR, 0, NULL);
                AppendMenu(hPopup, MF_STRING, ID_MENU_TERMINATE_PROCESS, L"结束进程");
                AppendMenu(hPopup, MF_STRING, ID_MENU_VIEW_PROCESS_MODULES, L"查看进程模块");
                TrackPopupMenu(hPopup, TPM_LEFTALIGN | TPM_RIGHTBUTTON, x, y, 0, m_hwnd, NULL);
                DestroyMenu(hPopup);
            }
        }
        return true;
    }
    return false;
}

void ProcessPage::UpdateStatus() {
    // Notify MainWindow to refresh status bar text
    PostMessage(m_hParent, WM_APP_UPDATE_STATUS, 0, 0);
}

LRESULT CALLBACK ProcessPage::ContainerWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto self = reinterpret_cast<ProcessPage*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (!self) return DefWindowProc(hwnd, msg, wParam, lParam);
    switch (msg) {
    case WM_NOTIFY: {
        LPNMHDR hdr = (LPNMHDR)lParam;
        if (hdr->hwndFrom == self->m_hwndList && hdr->code == LVN_COLUMNCLICK) {
            auto pnm = (NMLISTVIEW*)lParam;
            int column = pnm->iSubItem;
            bool ascending = self->m_sortAscending[column];
            SendMessage(self->m_hwndList, WM_SETREDRAW, FALSE, 0);
            self->m_sortCtx.column = column;
            self->m_sortCtx.ascending = ascending;
            self->m_sortCtx.hwndList = self->m_hwndList;
            ListView_SortItems(self->m_hwndList, CompareFunc, reinterpret_cast<LPARAM>(&self->m_sortCtx));
            SendMessage(self->m_hwndList, WM_SETREDRAW, TRUE, 0);
            self->m_sortAscending[column] = !ascending;
            // Update header sort arrow
            HWND hHeader = ListView_GetHeader(self->m_hwndList);
            if (hHeader) {
                int cols = Header_GetItemCount(hHeader);
                for (int i = 0; i < cols; ++i) {
                    HDITEM item{}; item.mask = HDI_FORMAT; Header_GetItem(hHeader, i, &item);
                    item.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
                    if (i == column) item.fmt |= (ascending ? HDF_SORTUP : HDF_SORTDOWN);
                    Header_SetItem(hHeader, i, &item);
                }
            }
            return 0;
        }
        break;
    }
    case WM_CONTEXTMENU: {
        HWND src = (HWND)wParam;
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        if (src == self->m_hwndList || src == self->m_hwnd) {
            POINT pt{ x, y };
            if (x == -1 && y == -1) {
                int sel = ListView_GetNextItem(self->m_hwndList, -1, LVNI_SELECTED);
                RECT rc{};
                if (sel != -1 && ListView_GetItemRect(self->m_hwndList, sel, &rc, LVIR_BOUNDS)) {
                    POINT ptClient{ rc.left, rc.bottom };
                    ClientToScreen(self->m_hwndList, &ptClient);
                    pt = ptClient;
                } else {
                    GetWindowRect(self->m_hwndList, &rc);
                    pt.x = rc.left + 10; pt.y = rc.top + 10;
                }
            } else {
                // update selection to item under cursor
                POINT ptClient{ pt.x, pt.y };
                ScreenToClient(self->m_hwndList, &ptClient);
                LVHITTESTINFO hti{}; hti.pt = ptClient;
                int hit = ListView_HitTest(self->m_hwndList, &hti);
                if (hit >= 0) {
                    ListView_SetItemState(self->m_hwndList, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
                    ListView_SetItemState(self->m_hwndList, hit, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
                }
            }
            // show context menu using RETURNCMD to handle command locally
            HMENU hPopup = CreatePopupMenu();
            if (hPopup) {
                AppendMenu(hPopup, MF_STRING, ID_MENU_REFRESH, L"刷新");
                AppendMenu(hPopup, MF_SEPARATOR, 0, NULL);
                AppendMenu(hPopup, MF_STRING, ID_MENU_TERMINATE_PROCESS, L"结束进程");
                AppendMenu(hPopup, MF_STRING, ID_MENU_VIEW_PROCESS_MODULES, L"查看进程模块");
                UINT cmd = TrackPopupMenu(hPopup, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, self->m_hwnd, NULL);
                DestroyMenu(hPopup);
                if (cmd) {
                    self->HandleCommand((WORD)cmd, 0);
                }
            }
            return 0;
        }
        break;
    }
    case WM_COMMAND: {
        WORD id = LOWORD(wParam), code = HIWORD(wParam);
        if (self->HandleCommand(id, code)) return 0;
        break;
    }
    }
    return CallWindowProc(self->m_prevProc, hwnd, msg, wParam, lParam);
}
