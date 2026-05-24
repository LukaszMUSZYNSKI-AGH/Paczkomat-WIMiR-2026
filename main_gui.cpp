// main_gui.cpp  –  Win32 GUI front-end for Paczkomat
// Code::Blocks + MinGW (g++ -std=c++17 -mwindows ... -lcomctl32)

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <cstdlib>    // wcstol
#include "Paczkomat.h"
#include "Skrytka.h"
#include "Paczka.h"

// ── Color palette ──────────────────────────────────────────────────────────
static const COLORREF COL_BG       = RGB(15,  17,  23);
static const COLORREF COL_PANEL    = RGB(24,  28,  38);
static const COLORREF COL_ACCENT   = RGB(255, 184,   0);
static const COLORREF COL_FREE     = RGB(39,  174,  96);
static const COLORREF COL_OCCUPIED = RGB(231,  76,  60);
static const COLORREF COL_TEXT     = RGB(236, 240, 241);
static const COLORREF COL_SUBTEXT  = RGB(127, 140, 141);
static const COLORREF COL_BORDER   = RGB(44,   62,  80);

// ── Control IDs ───────────────────────────────────────────────────────────
#define IDC_BTN_ODBIOR   101
#define IDC_BTN_NADANIE  102

#define IDC_OD_TEL       301
#define IDC_OD_KOD       302
#define IDC_OD_OK        303

#define IDC_NA_TEL       401
#define IDC_NA_KOD       402
#define IDC_NA_OK        404
#define IDC_NA_RA        501
#define IDC_NA_RB        502
#define IDC_NA_RC        503

// ── Globals ────────────────────────────────────────────────────────────────
static Paczkomat* g_paczkomat = nullptr;
static HFONT      g_fontTitle = nullptr;
static HFONT      g_fontBody  = nullptr;
static HFONT      g_fontSmall = nullptr;

// ── Helpers ────────────────────────────────────────────────────────────────
static std::wstring toWide(const std::string& s)
{
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, nullptr, 0);
    std::wstring w(n, 0);
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, &w[0], n);
    while (!w.empty() && w.back() == L'\0') w.pop_back();
    return w;
}

static std::string fromWide(const std::wstring& w)
{
    if (w.empty()) return "";
    int n = WideCharToMultiByte(CP_ACP, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string s(n, 0);
    WideCharToMultiByte(CP_ACP, 0, w.c_str(), -1, &s[0], n, nullptr, nullptr);
    while (!s.empty() && s.back() == '\0') s.pop_back();
    return s;
}

static std::string getWindowTextStr(HWND hw)
{
    wchar_t buf[256] = {};
    GetWindowText(hw, buf, 256);
    return fromWide(buf);
}

static int getWindowTextInt(HWND hw)
{
    wchar_t buf[64] = {};
    GetWindowText(hw, buf, 64);
    return (int)wcstol(buf, nullptr, 10);
}

static HFONT MakeFont(int height, bool bold, const wchar_t* face)
{
    return CreateFont(
        height, 0, 0, 0,
        bold ? FW_BOLD : FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS, face);
}

static void FillRoundRect(HDC dc, RECT r, int rx, COLORREF fill, COLORREF border)
{
    HBRUSH br = CreateSolidBrush(fill);
    HPEN   pn = CreatePen(PS_SOLID, 1, border);
    HBRUSH ob = (HBRUSH)SelectObject(dc, br);
    HPEN   op = (HPEN)SelectObject(dc, pn);
    RoundRect(dc, r.left, r.top, r.right, r.bottom, rx, rx);
    SelectObject(dc, ob);
    SelectObject(dc, op);
    DeleteObject(br);
    DeleteObject(pn);
}

// ── Locker grid ────────────────────────────────────────────────────────────
struct LockerCell { int nummer; char rozmiar; bool zajeta; RECT rc; };
static std::vector<LockerCell> g_cells;

static void RebuildCells(HWND hWnd, const std::vector<Skrytka>& skrytki)
{
    g_cells.clear();
    RECT cr; GetClientRect(hWnd, &cr);
    const int gridTop = 130, gridLeft = 20;
    const int cellW = 90, cellH = 90, gapX = 14, gapY = 14;
    int cols = (cr.right - gridLeft * 2 + gapX) / (cellW + gapX);
    if (cols < 1) cols = 1;

    int idx = 0;
    for (const Skrytka& s : skrytki) {
        LockerCell lc;
        lc.nummer  = s.getNumer();
        lc.rozmiar = s.getRozmiarSkrytki();
        lc.zajeta  = s.czyZajeta();
        int col = idx % cols, row = idx / cols;
        lc.rc.left   = gridLeft + col * (cellW + gapX);
        lc.rc.top    = gridTop  + row * (cellH + gapY);
        lc.rc.right  = lc.rc.left + cellW;
        lc.rc.bottom = lc.rc.top  + cellH;
        g_cells.push_back(lc);
        ++idx;
    }
}

static void DrawCell(HDC dc, const LockerCell& cell)
{
    COLORREF fill   = cell.zajeta ? COL_OCCUPIED : COL_FREE;
    COLORREF border = cell.zajeta ? RGB(192,57,43) : RGB(39,130,70);

    // Drop shadow
    RECT sh = cell.rc; OffsetRect(&sh, 3, 3);
    HBRUSH shb = CreateSolidBrush(RGB(0,0,0));
    HPEN   shp = CreatePen(PS_NULL,0,0);
    HBRUSH olb = (HBRUSH)SelectObject(dc, shb);
    HPEN   olp = (HPEN)SelectObject(dc, shp);
    RoundRect(dc, sh.left, sh.top, sh.right, sh.bottom, 10, 10);
    SelectObject(dc, olb); SelectObject(dc, olp);
    DeleteObject(shb); DeleteObject(shp);

    FillRoundRect(dc, cell.rc, 10, fill, border);

    int cx = (cell.rc.left + cell.rc.right)  / 2;
    int cy = (cell.rc.top  + cell.rc.bottom) / 2;

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(255,255,255));

    // Size badge top-right
    {
        HFONT old = (HFONT)SelectObject(dc, g_fontSmall);
        wchar_t badge[2] = { (wchar_t)cell.rozmiar, 0 };
        RECT br = { cell.rc.right-22, cell.rc.top+5, cell.rc.right-4, cell.rc.top+20 };
        DrawText(dc, badge, 1, &br, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        SelectObject(dc, old);
    }
    // Number
    {
        HFONT old = (HFONT)SelectObject(dc, g_fontBody);
        wchar_t num[8];
        swprintf(num, 8, L"%d", cell.nummer);
        RECT nr = { cell.rc.left, cy-18, cell.rc.right, cy+2 };
        DrawText(dc, num, -1, &nr, DT_CENTER|DT_SINGLELINE);
        SelectObject(dc, old);
    }
    // Status
    {
        HFONT old = (HFONT)SelectObject(dc, g_fontSmall);
        const wchar_t* status = cell.zajeta ? L"ZAJETA" : L"WOLNA";
        RECT sr = { cell.rc.left, cy+4, cell.rc.right, cy+20 };
        DrawText(dc, status, -1, &sr, DT_CENTER|DT_SINGLELINE);
        SelectObject(dc, old);
    }
}

// ── Odbior dialog ──────────────────────────────────────────────────────────
struct OdbiorData { std::string tel; int kod; bool ok; };
static OdbiorData g_od;

// Named CALLBACK – no lambda-as-WNDPROC
static HWND g_odTel = nullptr, g_odKod = nullptr;

static LRESULT CALLBACK OdbiorWndProc(HWND hw, UINT m, WPARAM wp, LPARAM lp)
{
    static HBRUSH s_bgBr   = nullptr;
    static HBRUSH s_panBr  = nullptr;

    switch (m) {
    case WM_CREATE:
        s_bgBr  = CreateSolidBrush(COL_BG);
        s_panBr = CreateSolidBrush(COL_PANEL);
        return 0;

    case WM_COMMAND:
        if (LOWORD(wp) == IDC_OD_OK) {
            g_od.tel = getWindowTextStr(g_odTel);
            g_od.kod = getWindowTextInt(g_odKod);
            g_od.ok  = true;
            DestroyWindow(hw);
        }
        return 0;

    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORSTATIC:
        SetBkColor((HDC)wp, COL_PANEL);
        SetTextColor((HDC)wp, COL_TEXT);
        return (LRESULT)s_panBr;

    case WM_CTLCOLORBTN:
        SetBkColor((HDC)wp, COL_ACCENT);
        SetTextColor((HDC)wp, RGB(0,0,0));
        return (LRESULT)s_panBr;

    case WM_ERASEBKGND: {
        RECT r; GetClientRect(hw, &r);
        FillRect((HDC)wp, &r, s_bgBr);
        return 1;
    }
    case WM_DESTROY:
        DeleteObject(s_bgBr);  s_bgBr  = nullptr;
        DeleteObject(s_panBr); s_panBr = nullptr;
        PostMessage((HWND)GetWindowLongPtr(hw, GWLP_USERDATA), WM_USER+1, 0, 0);
        return 0;
    }
    return DefWindowProc(hw, m, wp, lp);
}

static void ShowOdbiorDialog(HWND hParent)
{
    // Register window class once
    static bool reg = false;
    if (!reg) {
        WNDCLASSEX wc = {};
        wc.cbSize        = sizeof(wc);
        wc.lpfnWndProc   = OdbiorWndProc;
        wc.hInstance     = GetModuleHandle(nullptr);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
        wc.lpszClassName = L"OdbiorDlg";
        RegisterClassEx(&wc);
        reg = true;
    }

    HWND hDlg = CreateWindowEx(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        L"OdbiorDlg", L"Odbior paczki",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        0, 0, 340, 220,
        hParent, nullptr, GetModuleHandle(nullptr), nullptr);

    SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)hParent);

    // Centre over parent
    RECT pr, dr;
    GetWindowRect(hParent, &pr);
    GetWindowRect(hDlg,    &dr);
    SetWindowPos(hDlg, nullptr,
        pr.left + (pr.right-pr.left)/2 - (dr.right-dr.left)/2,
        pr.top  + (pr.bottom-pr.top)/2 - (dr.bottom-dr.top)/2,
        0, 0, SWP_NOSIZE|SWP_NOZORDER);

    auto mkStatic = [&](const wchar_t* t, int x, int y, int w, int h) {
        HWND hw = CreateWindowEx(0, L"STATIC", t, WS_CHILD|WS_VISIBLE|SS_LEFT,
            x, y, w, h, hDlg, nullptr, nullptr, nullptr);
        SendMessage(hw, WM_SETFONT, (WPARAM)g_fontSmall, TRUE);
    };
    auto mkEdit = [&](int id, int x, int y, int w, int h, DWORD ex=0) -> HWND {
        HWND hw = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL|ex,
            x, y, w, h, hDlg, (HMENU)(size_t)id, nullptr, nullptr);
        SendMessage(hw, WM_SETFONT, (WPARAM)g_fontBody, TRUE);
        return hw;
    };
    auto mkBtn = [&](const wchar_t* t, int id, int x, int y, int w, int h) {
        HWND hw = CreateWindowEx(0, L"BUTTON", t,
            WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,
            x, y, w, h, hDlg, (HMENU)(size_t)id, nullptr, nullptr);
        SendMessage(hw, WM_SETFONT, (WPARAM)g_fontBody, TRUE);
    };

    mkStatic(L"Numer telefonu odbiorcy:", 20, 20, 200, 18);
    g_odTel = mkEdit(IDC_OD_TEL, 20, 42, 290, 28);
    mkStatic(L"Kod odbioru:", 20, 84, 200, 18);
    g_odKod = mkEdit(IDC_OD_KOD, 20, 106, 290, 28, ES_NUMBER);
    mkBtn(L"Odbierz", IDC_OD_OK, 100, 155, 130, 34);

    g_od.ok = false;
    ShowWindow(hDlg, SW_SHOW);
    UpdateWindow(hDlg);

    MSG msg;
    while (IsWindow(hDlg) && GetMessage(&msg, nullptr, 0, 0)) {
        if (!IsDialogMessage(hDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

// ── Nadanie dialog ─────────────────────────────────────────────────────────
struct NadanieData { std::string tel; int kod; char rozmiar; bool ok; };
static NadanieData g_na;

static HWND g_naTel = nullptr, g_naKod = nullptr;
static HWND g_naRA  = nullptr, g_naRB  = nullptr, g_naRC = nullptr;

static LRESULT CALLBACK NadanieWndProc(HWND hw, UINT m, WPARAM wp, LPARAM lp)
{
    static HBRUSH s_bgBr  = nullptr;
    static HBRUSH s_panBr = nullptr;

    switch (m) {
    case WM_CREATE:
        s_bgBr  = CreateSolidBrush(COL_BG);
        s_panBr = CreateSolidBrush(COL_PANEL);
        return 0;

    case WM_COMMAND:
        if (LOWORD(wp) == IDC_NA_OK) {
            g_na.tel = getWindowTextStr(g_naTel);
            g_na.kod = getWindowTextInt(g_naKod);
            g_na.rozmiar =
                (SendMessage(g_naRB, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 'B' :
                (SendMessage(g_naRC, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 'C' : 'A';
            g_na.ok = true;
            DestroyWindow(hw);
        }
        return 0;

    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORSTATIC:
        SetBkColor((HDC)wp, COL_PANEL);
        SetTextColor((HDC)wp, COL_TEXT);
        return (LRESULT)s_panBr;

    case WM_CTLCOLORBTN:
        SetBkColor((HDC)wp, COL_PANEL);
        SetTextColor((HDC)wp, COL_TEXT);
        return (LRESULT)s_panBr;

    case WM_ERASEBKGND: {
        RECT r; GetClientRect(hw, &r);
        FillRect((HDC)wp, &r, s_bgBr);
        return 1;
    }
    case WM_DESTROY:
        DeleteObject(s_bgBr);  s_bgBr  = nullptr;
        DeleteObject(s_panBr); s_panBr = nullptr;
        PostMessage((HWND)GetWindowLongPtr(hw, GWLP_USERDATA), WM_USER+2, 0, 0);
        return 0;
    }
    return DefWindowProc(hw, m, wp, lp);
}

static void ShowNadanieDialog(HWND hParent)
{
    static bool reg = false;
    if (!reg) {
        WNDCLASSEX wc = {};
        wc.cbSize        = sizeof(wc);
        wc.lpfnWndProc   = NadanieWndProc;
        wc.hInstance     = GetModuleHandle(nullptr);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
        wc.lpszClassName = L"NadanieDlg";
        RegisterClassEx(&wc);
        reg = true;
    }

    HWND hDlg = CreateWindowEx(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        L"NadanieDlg", L"Nadanie paczki",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        0, 0, 340, 270,
        hParent, nullptr, GetModuleHandle(nullptr), nullptr);

    SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)hParent);

    RECT pr, dr;
    GetWindowRect(hParent, &pr);
    GetWindowRect(hDlg,    &dr);
    SetWindowPos(hDlg, nullptr,
        pr.left + (pr.right-pr.left)/2 - (dr.right-dr.left)/2,
        pr.top  + (pr.bottom-pr.top)/2 - (dr.bottom-dr.top)/2,
        0, 0, SWP_NOSIZE|SWP_NOZORDER);

    auto mkStatic = [&](const wchar_t* t, int x, int y, int w, int h) {
        HWND hw = CreateWindowEx(0, L"STATIC", t, WS_CHILD|WS_VISIBLE|SS_LEFT,
            x, y, w, h, hDlg, nullptr, nullptr, nullptr);
        SendMessage(hw, WM_SETFONT, (WPARAM)g_fontSmall, TRUE);
    };
    auto mkEdit = [&](int id, int x, int y, int w, int h, DWORD ex=0) -> HWND {
        HWND hw = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL|ex,
            x, y, w, h, hDlg, (HMENU)(size_t)id, nullptr, nullptr);
        SendMessage(hw, WM_SETFONT, (WPARAM)g_fontBody, TRUE);
        return hw;
    };
    auto mkRadio = [&](const wchar_t* t, int id, int x, DWORD extra=0) -> HWND {
        HWND hw = CreateWindowEx(0, L"BUTTON", t,
            WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTORADIOBUTTON|extra,
            x, 168, 85, 22, hDlg, (HMENU)(size_t)id, nullptr, nullptr);
        SendMessage(hw, WM_SETFONT, (WPARAM)g_fontSmall, TRUE);
        return hw;
    };
    auto mkBtn = [&](const wchar_t* t, int id, int x, int y, int w, int h) {
        HWND hw = CreateWindowEx(0, L"BUTTON", t,
            WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,
            x, y, w, h, hDlg, (HMENU)(size_t)id, nullptr, nullptr);
        SendMessage(hw, WM_SETFONT, (WPARAM)g_fontBody, TRUE);
    };

    mkStatic(L"Numer telefonu:", 20, 20, 200, 18);
    g_naTel = mkEdit(IDC_NA_TEL, 20, 42, 290, 28);
    mkStatic(L"Kod odbioru:", 20, 84, 200, 18);
    g_naKod = mkEdit(IDC_NA_KOD, 20, 106, 290, 28, ES_NUMBER);
    mkStatic(L"Rozmiar paczki:", 20, 148, 200, 18);

    g_naRA = mkRadio(L"A  (maly)",   IDC_NA_RA,  20, WS_GROUP);
    g_naRB = mkRadio(L"B  (sredni)", IDC_NA_RB, 112);
    g_naRC = mkRadio(L"C  (duzy)",   IDC_NA_RC, 210);
    SendMessage(g_naRA, BM_SETCHECK, BST_CHECKED, 0);

    mkBtn(L"Nadaj", IDC_NA_OK, 100, 210, 130, 34);

    g_na.ok = false;
    ShowWindow(hDlg, SW_SHOW);
    UpdateWindow(hDlg);

    MSG msg;
    while (IsWindow(hDlg) && GetMessage(&msg, nullptr, 0, 0)) {
        if (!IsDialogMessage(hDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

// ── Main window drawing ────────────────────────────────────────────────────
static void DrawHeader(HDC dc, RECT cr)
{
    HBRUSH bgBr = CreateSolidBrush(COL_BG);
    FillRect(dc, &cr, bgBr);
    DeleteObject(bgBr);

    // Yellow top bar
    RECT topBar = {0, 0, cr.right, 6};
    HBRUSH acBr = CreateSolidBrush(COL_ACCENT);
    FillRect(dc, &topBar, acBr);
    DeleteObject(acBr);

    // Header panel
    RECT logoBox = {14, 14, cr.right-14, 120};
    FillRoundRect(dc, logoBox, 12, COL_PANEL, COL_BORDER);

    SetBkMode(dc, TRANSPARENT);

    // Title
    SelectObject(dc, g_fontTitle);
    SetTextColor(dc, COL_ACCENT);
    RECT titleRc = {24, 22, cr.right-24, 70};
    DrawText(dc, L"PACZKOMAT", -1, &titleRc, DT_LEFT|DT_SINGLELINE|DT_VCENTER);

    // Address line
    SelectObject(dc, g_fontSmall);
    SetTextColor(dc, COL_SUBTEXT);
    RECT subRc = {24, 68, cr.right-24, 90};
    DrawText(dc, L"ul. Przykladowa 1   |   KRK01", -1, &subRc, DT_LEFT|DT_SINGLELINE);

    // Legend – green dot
    HBRUSH gb = CreateSolidBrush(COL_FREE);
    HPEN   gp = CreatePen(PS_NULL, 0, 0);
    HBRUSH ob = (HBRUSH)SelectObject(dc, gb);
    HPEN   op = (HPEN)SelectObject(dc, gp);
    Ellipse(dc, 24, 96, 36, 108);
    SelectObject(dc, ob); SelectObject(dc, op);
    DeleteObject(gb); DeleteObject(gp);
    SetTextColor(dc, COL_SUBTEXT);
    RECT lr1 = {40, 92, 110, 112};
    DrawText(dc, L"Wolna", -1, &lr1, DT_LEFT|DT_SINGLELINE|DT_VCENTER);

    // Legend – red dot
    HBRUSH rb = CreateSolidBrush(COL_OCCUPIED);
    HPEN   rp = CreatePen(PS_NULL, 0, 0);
    ob = (HBRUSH)SelectObject(dc, rb);
    op = (HPEN)SelectObject(dc, rp);
    Ellipse(dc, 116, 96, 128, 108);
    SelectObject(dc, ob); SelectObject(dc, op);
    DeleteObject(rb); DeleteObject(rp);
    RECT lr2 = {132, 92, 210, 112};
    DrawText(dc, L"Zajeta", -1, &lr2, DT_LEFT|DT_SINGLELINE|DT_VCENTER);
}

static void OnPaint(HWND hWnd)
{
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(hWnd, &ps);
    RECT cr; GetClientRect(hWnd, &cr);

    // Double-buffer
    HDC     memDC  = CreateCompatibleDC(dc);
    HBITMAP memBmp = CreateCompatibleBitmap(dc, cr.right, cr.bottom);
    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

    DrawHeader(memDC, cr);
    for (const auto& cell : g_cells)
        DrawCell(memDC, cell);

    BitBlt(dc, 0, 0, cr.right, cr.bottom, memDC, 0, 0, SRCCOPY);
    SelectObject(memDC, oldBmp);
    DeleteObject(memBmp);
    DeleteDC(memDC);
    EndPaint(hWnd, &ps);
}

// ── Main window proc ───────────────────────────────────────────────────────
static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HBRUSH s_btnBr = nullptr;

    switch (msg) {
    case WM_CREATE: {
        s_btnBr = CreateSolidBrush(COL_ACCENT);
        RECT cr; GetClientRect(hWnd, &cr);
        int bw = 150, bh = 40;
        int by = cr.bottom - bh - 16;
        int cx = (cr.right - (bw*2 + 16)) / 2;

        auto makeBtn = [&](const wchar_t* t, int id, int x) {
            HWND hw = CreateWindowEx(0, L"BUTTON", t,
                WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
                x, by, bw, bh, hWnd, (HMENU)(size_t)id, nullptr, nullptr);
            SendMessage(hw, WM_SETFONT, (WPARAM)g_fontBody, TRUE);
        };
        makeBtn(L"Odbierz paczke", IDC_BTN_ODBIOR,  cx);
        makeBtn(L"Nadaj paczke",   IDC_BTN_NADANIE, cx + bw + 16);
        return 0;
    }

    case WM_SIZE: {
        RECT cr; GetClientRect(hWnd, &cr);
        int bw = 150, bh = 40;
        int by = cr.bottom - bh - 16;
        int cx = (cr.right - (bw*2 + 16)) / 2;
        SetWindowPos(GetDlgItem(hWnd, IDC_BTN_ODBIOR),  nullptr, cx,          by, bw, bh, SWP_NOZORDER);
        SetWindowPos(GetDlgItem(hWnd, IDC_BTN_NADANIE), nullptr, cx+bw+16,    by, bw, bh, SWP_NOZORDER);
        if (g_paczkomat) RebuildCells(hWnd, g_paczkomat->getSkrytki());
        InvalidateRect(hWnd, nullptr, FALSE);
        return 0;
    }

    case WM_PAINT:
        OnPaint(hWnd);
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_CTLCOLORBTN:
        SetBkColor((HDC)wParam, COL_ACCENT);
        SetTextColor((HDC)wParam, RGB(0,0,0));
        return (LRESULT)s_btnBr;

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == IDC_BTN_ODBIOR) {
            ShowOdbiorDialog(hWnd);
            if (g_od.ok) {
                bool found = g_paczkomat->odbiorGUI(g_od.tel, g_od.kod);
                MessageBox(hWnd,
                    found ? L"Paczka wydana. Dziekujemy!" :
                            L"Nie znaleziono paczki o podanym kodzie i numerze telefonu.",
                    L"Odbior",
                    MB_OK | (found ? MB_ICONINFORMATION : MB_ICONWARNING));
                RebuildCells(hWnd, g_paczkomat->getSkrytki());
                InvalidateRect(hWnd, nullptr, FALSE);
            }
        }
        else if (id == IDC_BTN_NADANIE) {
            ShowNadanieDialog(hWnd);
            if (g_na.ok) {
                bool placed = g_paczkomat->nadanieGUI(g_na.tel, g_na.kod, g_na.rozmiar);
                if (placed) {
                    MessageBox(hWnd, L"Paczka przyjeta do skrytki!", L"Nadanie", MB_OK|MB_ICONINFORMATION);
                } else {
                    wchar_t buf[128];
                    swprintf(buf, 128, L"Brak wolnych skrytek rozmiaru %c.", (wchar_t)g_na.rozmiar);
                    MessageBox(hWnd, buf, L"Nadanie", MB_OK|MB_ICONWARNING);
                }
                RebuildCells(hWnd, g_paczkomat->getSkrytki());
                InvalidateRect(hWnd, nullptr, FALSE);
            }
        }
        return 0;
    }

    case WM_DESTROY:
        DeleteObject(s_btnBr);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// ── WinMain ────────────────────────────────────────────────────────────────
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nShow)
{
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);

    g_fontTitle = MakeFont(32, true,  L"Segoe UI");
    g_fontBody  = MakeFont(16, false, L"Segoe UI");
    g_fontSmall = MakeFont(13, false, L"Segoe UI");

    std::vector<Skrytka> skrytki = {
        Skrytka(1,'A'), Skrytka(2,'A'), Skrytka(3,'A'),
        Skrytka(4,'B'), Skrytka(5,'B'), Skrytka(6,'B'),
        Skrytka(7,'C'), Skrytka(8,'C')
    };
    Paczkomat pm("ul. Przykladowa 1", "KRK01", skrytki);
    g_paczkomat = &pm;

    WNDCLASSEX wc = {};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = MainWndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(COL_BG);
    wc.lpszClassName = L"PaczkomatWnd";
    wc.hIcon         = LoadIcon(nullptr, IDI_APPLICATION);
    RegisterClassEx(&wc);

    HWND hWnd = CreateWindowEx(
        0, L"PaczkomatWnd", L"Paczkomat - KRK01",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 620, 580,
        nullptr, nullptr, hInst, nullptr);

    RebuildCells(hWnd, g_paczkomat->getSkrytki());
    ShowWindow(hWnd, nShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DeleteObject(g_fontTitle);
    DeleteObject(g_fontBody);
    DeleteObject(g_fontSmall);
    return (int)msg.wParam;
}
