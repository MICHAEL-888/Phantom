#pragma once
#include <windows.h>
#include <commctrl.h>
#include <string>

class ListViewEx {
public:
    explicit ListViewEx(HWND hwnd = nullptr) : m_hwnd(hwnd) {}
    void Attach(HWND hwnd) { m_hwnd = hwnd; }
    HWND GetHwnd() const { return m_hwnd; }

    void SetExtendedStyles(DWORD styles) {
        // Use Ex variant to set only provided bits and preserve others
        ListView_SetExtendedListViewStyleEx(m_hwnd, styles, styles);
    }

    void InsertColumn(int index, const wchar_t* text, int width, int fmt) {
        LVCOLUMN col{};
        col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;
        col.pszText = const_cast<wchar_t*>(text);
        col.cx = width;
        col.fmt = fmt;
        ListView_InsertColumn(m_hwnd, index, &col);
    }

    void BeginUpdate() { SendMessage(m_hwnd, WM_SETREDRAW, FALSE, 0); }
    void EndUpdate() { SendMessage(m_hwnd, WM_SETREDRAW, TRUE, 0); InvalidateRect(m_hwnd, NULL, TRUE); }
    void Clear() { ListView_DeleteAllItems(m_hwnd); }

    void ApplyHeaderSortArrow(int column, bool ascending) {
        HWND hHeader = ListView_GetHeader(m_hwnd);
        int cols = Header_GetItemCount(hHeader);
        for (int i = 0; i < cols; ++i) {
            HDITEM item{};
            item.mask = HDI_FORMAT;
            Header_GetItem(hHeader, i, &item);
            item.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
            if (i == column)
                item.fmt |= ascending ? HDF_SORTUP : HDF_SORTDOWN;
            Header_SetItem(hHeader, i, &item);
        }
    }

private:
    HWND m_hwnd{};
};
