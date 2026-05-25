// main_gui.cpp  –  Win32 GUI front-end dla Paczkomatu
// Kompilacja (Code::Blocks / MinGW):
//   g++ -std=c++17 -mwindows main_gui.cpp Paczka.cpp Skrytka.cpp Paczkomat.cpp -lcomctl32 -o Paczkomat.exe
//
// Role i PINy (stale w kodzie):
//   Serwisant : PIN 1111  – pelny podglad + reset skrytek
//   Kurier    : PIN 2222  – nadanie bez kodu (generuje system)

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
#include <cstdlib>
#include "Paczkomat.h"
#include "Skrytka.h"
#include "Paczka.h"

// ─── PINy rol ────────────────────────────────────────────────────────────────
static const int PIN_SERWISANT = 1111;
static const int PIN_KURIER    = 2222;

// ─── Paleta kolorow ──────────────────────────────────────────────────────────
static const COLORREF COL_BG       = RGB( 15,  17,  23);
static const COLORREF COL_PANEL    = RGB( 24,  28,  38);
static const COLORREF COL_PANEL2   = RGB( 32,  38,  52);
static const COLORREF COL_ACCENT   = RGB(255, 184,   0);
static const COLORREF COL_KURIER   = RGB( 52, 152, 219);
static const COLORREF COL_SERWIS   = RGB(231,  76,  60);
static const COLORREF COL_FREE     = RGB( 39, 174,  96);
static const COLORREF COL_OCCUPIED = RGB(231,  76,  60);
static const COLORREF COL_TEXT     = RGB(236, 240, 241);
static const COLORREF COL_SUBTEXT  = RGB(127, 140, 141);
static const COLORREF COL_BORDER   = RGB( 44,  62,  80);

// ─── IDy kontrolek ───────────────────────────────────────────────────────────
#define IDC_BTN_ODBIOR      101
#define IDC_BTN_NADANIE     102
#define IDC_BTN_KURIER      103
#define IDC_BTN_SERWISANT   104
#define IDC_BTN_WYLOGUJ     105

#define IDC_OD_TEL          201
#define IDC_OD_KOD          202
#define IDC_OD_OK           203
#define IDC_OD_BACK         204

#define IDC_NA_TEL          301
#define IDC_NA_KOD          302
#define IDC_NA_RA           303
#define IDC_NA_RB           304
#define IDC_NA_RC           305
#define IDC_NA_OK           306
#define IDC_NA_BACK         307

#define IDC_LOG_PIN         401
#define IDC_LOG_OK          402
#define IDC_LOG_CANCEL      403

// klawiatura: 500+0..9=cyfry, 500+10=backspace, OK=osobny per widok
#define IDC_NUM_BASE        500

#define IDC_KUR_TEL         601
#define IDC_KUR_RA          602
#define IDC_KUR_RB          603
#define IDC_KUR_RC          604
#define IDC_KUR_OK          605
#define IDC_KUR_BACK        606

#define IDC_SRV_LIST        701
#define IDC_SRV_RESET       702
#define IDC_SRV_BACK        703
#define IDC_POTWIERDZ       801   // "Zamknalem skrytke"

// ─── Tryb widoku / rola ───────────────────────────────────────────────────────
enum class Widok {
    GLOWNY,
    ODBIOR_TEL, ODBIOR_KOD,
    NADANIE_TEL, NADANIE_KOD, NADANIE_ROZMIAR,
    LOGOWANIE_KURIER, LOGOWANIE_SERWISANT,
    KURIER_TEL, KURIER_ROZMIAR,
    SERWISANT,
    POTWIERDZENIE   // czeka na klikniecie "Zamknalem skrytke"
};
enum class Rola { BRAK, KURIER, SERWISANT };

// ─── Globale ─────────────────────────────────────────────────────────────────
static Paczkomat* g_pm      = nullptr;
static HFONT      g_fTitle  = nullptr;
static HFONT      g_fBody   = nullptr;
static HFONT      g_fSmall  = nullptr;

static Widok g_widok = Widok::GLOWNY;
static Rola  g_rola  = Rola::BRAK;
static Rola  g_logowanieDoRoli = Rola::BRAK;

static std::wstring g_bufTel;
static std::wstring g_bufKod;
static char         g_rozmiar = 'A';
static std::wstring g_odTel, g_naKod, g_naTel, g_kurTel;
static std::wstring g_komunikat;
static bool         g_komunikatOK = true;

// ─── Pomocnicze ──────────────────────────────────────────────────────────────
static std::wstring ToWide(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, nullptr, 0);
    std::wstring w(n, 0);
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, &w[0], n);
    while (!w.empty() && w.back()==L'\0') w.pop_back();
    return w;
}
static std::string FromWide(const std::wstring& w) {
    if (w.empty()) return "";
    int n = WideCharToMultiByte(CP_ACP, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string s(n, 0);
    WideCharToMultiByte(CP_ACP, 0, w.c_str(), -1, &s[0], n, nullptr, nullptr);
    while (!s.empty() && s.back()=='\0') s.pop_back();
    return s;
}
static HFONT MakeFont(int h, bool bold, const wchar_t* face) {
    return CreateFont(h,0,0,0, bold?FW_BOLD:FW_NORMAL,
        FALSE,FALSE,FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH|FF_SWISS, face);
}
static void FillRR(HDC dc, RECT r, int rx, COLORREF fill, COLORREF bord) {
    HBRUSH br = CreateSolidBrush(fill);
    HPEN   pn = CreatePen(PS_SOLID,1,bord);
    HBRUSH ob=(HBRUSH)SelectObject(dc,br);
    HPEN   op_=(HPEN)SelectObject(dc,pn);
    RoundRect(dc,r.left,r.top,r.right,r.bottom,rx,rx);
    SelectObject(dc,ob); SelectObject(dc,op_);
    DeleteObject(br); DeleteObject(pn);
}
static COLORREF RolaKolor() {
    if (g_rola==Rola::KURIER)    return COL_KURIER;
    if (g_rola==Rola::SERWISANT) return COL_SERWIS;
    return COL_ACCENT;
}
static const wchar_t* RolaNazwa() {
    if (g_rola==Rola::KURIER)    return L"KURIER";
    if (g_rola==Rola::SERWISANT) return L"SERWISANT";
    return L"KLIENT";
}

// ─── Kafelek skrytki ─────────────────────────────────────────────────────────
struct Kafelek {
    int          id;
    char         rozmiar;
    bool         zajeta;
    RECT         rc;
    std::wstring tel;   // wypelnione tylko gdy zajeta
    int          kod;   // wypelnione tylko gdy zajeta
};
static std::vector<Kafelek> g_kafle;

// numer skrytki podswietlonej po odebraniu / wyswietlonej klientowi (-1 = brak)
static int g_podswietlona = -1;

static void OdbudujKafle(HWND hw) {
    g_kafle.clear();
    const auto& sk = g_pm->getSkrytki();
    RECT cr; GetClientRect(hw,&cr);
    const int TOP=152,LEFT=20,W=110,H=110,GX=10,GY=10;
    int cols = std::max(1,(int)(cr.right-LEFT*2+GX)/(W+GX));
    int idx=0;
    for (const Skrytka& s : sk) {
        Kafelek k;
        k.id=s.getNumer(); k.rozmiar=s.getRozmiarSkrytki(); k.zajeta=s.czyZajeta();
        k.tel=L""; k.kod=0;
        if (k.zajeta && s.getPaczka().has_value()) {
            k.tel = ToWide(s.getPaczka()->getTelOdbiorcy());
            k.kod = s.getPaczka()->getKodPaczki();
        }
        int col=idx%cols, row=idx/cols;
        k.rc={LEFT+col*(W+GX), TOP+row*(H+GY), LEFT+col*(W+GX)+W, TOP+row*(H+GY)+H};
        g_kafle.push_back(k);
        ++idx;
    }
}

// tryb  = 0 : klient – wszystko ukryte
// tryb  = 1 : klient – tylko ta jedna skrytka podswietlona (po odebraniu)
// tryb  = 2 : serwisant – pełne info na kafle
// tryb  = 3 : kurier – wolna/zajeta bez kodu i telefonu
static void RysujKafelek(HDC dc, const Kafelek& k, int tryb) {
    bool ujawnij  = (tryb == 2);
    bool highlight = (tryb == 1);
    bool kurierView = (tryb == 3);

    COLORREF fill, bord;
    if (highlight) {
        // podswietlona skrytka klienta – zielona ramka, reszta taka jak zajeta
        fill = RGB(20, 60, 30);
        bord = COL_FREE;
    } else if (kurierView) {
        // kurier – koloruje skrytki wedlug dostepnosci
        fill = k.zajeta ? RGB(50,20,20) : RGB(18,40,25);
        bord = k.zajeta ? COL_OCCUPIED : COL_FREE;
    } else if (!ujawnij) {
        fill = RGB(20,22,30); bord = COL_BORDER;
    } else {
        fill = k.zajeta ? COL_OCCUPIED : COL_FREE;
        bord = k.zajeta ? RGB(192,57,43) : RGB(39,130,70);
    }

    // cień
    RECT sh=k.rc; OffsetRect(&sh,3,3);
    {
        HBRUSH sb=CreateSolidBrush(RGB(0,0,0)); HPEN sp=CreatePen(PS_NULL,0,0);
        HBRUSH ob=(HBRUSH)SelectObject(dc,sb); HPEN op=(HPEN)SelectObject(dc,sp);
        RoundRect(dc,sh.left,sh.top,sh.right,sh.bottom,10,10);
        SelectObject(dc,ob); SelectObject(dc,op); DeleteObject(sb); DeleteObject(sp);
    }
    FillRR(dc,k.rc,10,fill,bord);
    SetBkMode(dc,TRANSPARENT); SetTextColor(dc,COL_TEXT);

    int cy = (k.rc.top+k.rc.bottom)/2;

    // badge rozmiaru (zawsze)
    {
        auto old=(HFONT)SelectObject(dc,g_fSmall);
        wchar_t b[2]={(wchar_t)k.rozmiar,0};
        RECT br={k.rc.right-22,k.rc.top+5,k.rc.right-4,k.rc.top+20};
        DrawText(dc,b,1,&br,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        SelectObject(dc,old);
    }

    if (ujawnij) {
        // ── SERWISANT: numer + status + tel (ostatnie 4) + kod ──────────
        {
            auto old=(HFONT)SelectObject(dc,g_fBody);
            wchar_t num[8]; swprintf(num,8,L"%d",k.id);
            RECT nr={k.rc.left,k.rc.top+8,k.rc.right,k.rc.top+34};
            SetTextColor(dc,COL_TEXT);
            DrawText(dc,num,-1,&nr,DT_CENTER|DT_SINGLELINE);
            SelectObject(dc,old);
        }
        {
            auto old=(HFONT)SelectObject(dc,g_fSmall);
            RECT sr={k.rc.left,k.rc.top+36,k.rc.right,k.rc.top+52};
            SetTextColor(dc,COL_TEXT);
            DrawText(dc,k.zajeta?L"ZAJETA":L"WOLNA",-1,&sr,DT_CENTER|DT_SINGLELINE);
            SelectObject(dc,old);
        }
        if (k.zajeta && !k.tel.empty()) {
            auto old=(HFONT)SelectObject(dc,g_fSmall);
            SetTextColor(dc,COL_TEXT);
            std::wstring telSkrot = L"...";
            if (k.tel.size()>=4) telSkrot += k.tel.substr(k.tel.size()-4);
            else telSkrot += k.tel;
            RECT tr={k.rc.left+2,k.rc.top+56,k.rc.right-2,k.rc.top+72};
            DrawText(dc,telSkrot.c_str(),-1,&tr,DT_CENTER|DT_SINGLELINE);
            wchar_t kodBuf[16]; swprintf(kodBuf,16,L"[%d]",k.kod);
            RECT kr={k.rc.left+2,k.rc.top+73,k.rc.right-2,k.rc.top+89};
            DrawText(dc,kodBuf,-1,&kr,DT_CENTER|DT_SINGLELINE);
            SelectObject(dc,old);
        }
    } else if (highlight) {
        // ── KLIENT po odebraniu: numer + "OTWARTA" ──────────────────────
        {
            auto old=(HFONT)SelectObject(dc,g_fBody);
            wchar_t num[8]; swprintf(num,8,L"%d",k.id);
            RECT nr={k.rc.left,cy-18,k.rc.right,cy+2};
            SetTextColor(dc,COL_TEXT);
            DrawText(dc,num,-1,&nr,DT_CENTER|DT_SINGLELINE);
            SelectObject(dc,old);
        }
        {
            auto old=(HFONT)SelectObject(dc,g_fSmall);
            SetTextColor(dc,COL_TEXT);
            RECT sr={k.rc.left,cy+4,k.rc.right,cy+20};
            DrawText(dc,L"OTWARTA",-1,&sr,DT_CENTER|DT_SINGLELINE);
            SelectObject(dc,old);
        }
    } else if (kurierView) {
        // ── KURIER: numer + WOLNA/ZAJETA (bez tel i kodu) ──────────────
        {
            auto old=(HFONT)SelectObject(dc,g_fBody);
            wchar_t num[8]; swprintf(num,8,L"%d",k.id);
            RECT nr={k.rc.left,cy-18,k.rc.right,cy+2};
            SetTextColor(dc,COL_TEXT);
            DrawText(dc,num,-1,&nr,DT_CENTER|DT_SINGLELINE);
            SelectObject(dc,old);
        }
        {
            auto old=(HFONT)SelectObject(dc,g_fSmall);
            COLORREF sc = k.zajeta ? COL_OCCUPIED : COL_FREE;
            SetTextColor(dc,sc);
            RECT sr={k.rc.left,cy+4,k.rc.right,cy+20};
            DrawText(dc,k.zajeta?L"ZAJETA":L"WOLNA",-1,&sr,DT_CENTER|DT_SINGLELINE);
            SelectObject(dc,old);
        }
    } else {
        // ── KLIENT zwykły: numer bialy, status ukryty ───────────────────
        {
            auto old=(HFONT)SelectObject(dc,g_fBody);
            wchar_t num[8]; swprintf(num,8,L"%d",k.id);
            RECT nr={k.rc.left,cy-18,k.rc.right,cy+2};
            SetTextColor(dc,COL_TEXT);
            DrawText(dc,num,-1,&nr,DT_CENTER|DT_SINGLELINE);
            SelectObject(dc,old);
        }
        {
            auto old=(HFONT)SelectObject(dc,g_fSmall);
            SetTextColor(dc,RGB(50,55,70));
            RECT sr={k.rc.left,cy+4,k.rc.right,cy+20};
            DrawText(dc,L"------",-1,&sr,DT_CENTER|DT_SINGLELINE);
            SelectObject(dc,old);
        }
    }
}

// ─── Naglowek ────────────────────────────────────────────────────────────────
static void RysujNaglowek(HDC dc, RECT cr) {
    HBRUSH bg=CreateSolidBrush(COL_BG); FillRect(dc,&cr,bg); DeleteObject(bg);
    COLORREF ak=RolaKolor();
    RECT top={0,0,cr.right,6}; HBRUSH ab=CreateSolidBrush(ak); FillRect(dc,&top,ab); DeleteObject(ab);
    RECT hbox={14,14,cr.right-14,140}; FillRR(dc,hbox,12,COL_PANEL,COL_BORDER);
    SetBkMode(dc,TRANSPARENT);
    SelectObject(dc,g_fTitle); SetTextColor(dc,ak);
    RECT tr={24,20,cr.right-24,68}; DrawText(dc,L"PACZKOMAT",-1,&tr,DT_LEFT|DT_SINGLELINE|DT_VCENTER);
    SelectObject(dc,g_fSmall); SetTextColor(dc,COL_SUBTEXT);
    RECT ar={24,68,cr.right-24,88}; DrawText(dc,L"ul. Czarnowiejska 30/2   |   KRK01",-1,&ar,DT_LEFT|DT_SINGLELINE);
    // badge roli
    RECT br={24,94,144,118}; FillRR(dc,br,8,ak,ak);
    SetTextColor(dc,RGB(0,0,0)); SelectObject(dc,g_fSmall);
    DrawText(dc,RolaNazwa(),-1,&br,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    // komunikat
    if (!g_komunikat.empty()) {
        COLORREF kc=g_komunikatOK?COL_FREE:COL_OCCUPIED;
        RECT km={154,94,cr.right-24,118}; FillRR(dc,km,6,COL_PANEL2,kc);
        SetTextColor(dc,kc); SelectObject(dc,g_fSmall);
        DrawText(dc,g_komunikat.c_str(),-1,&km,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    }
}

static void OnPaint(HWND hw) {
    PAINTSTRUCT ps; HDC dc=BeginPaint(hw,&ps);
    RECT cr; GetClientRect(hw,&cr);
    HDC mem=CreateCompatibleDC(dc);
    HBITMAP bmp=CreateCompatibleBitmap(dc,cr.right,cr.bottom);
    auto oldb=(HBITMAP)SelectObject(mem,bmp);

    // panel aktywny = klawiatura lub formularz (nie GLOWNY i nie SERWISANT)
    bool panelAktywny = (g_widok != Widok::GLOWNY && g_widok != Widok::SERWISANT);

    RysujNaglowek(mem,cr);

    if (!panelAktywny) {
        for (const auto& k : g_kafle) {
            int tryb = 0;
            if (g_rola == Rola::SERWISANT) {
                tryb = 2;
            } else if (g_rola == Rola::KURIER) {
                tryb = 3;  // kurier: wolne/zajete bez kodu i telefonu
            } else if (k.id == g_podswietlona) {
                tryb = 1;
            }
            RysujKafelek(mem, k, tryb);
        }
    } else {
        // zakryj obszar kafli jednolitym tlem – od dolnej krawedzi naglowka
        RECT obszar = { 0, 142, cr.right, cr.bottom };
        HBRUSH bgBr = CreateSolidBrush(COL_BG);
        FillRect(mem, &obszar, bgBr);
        DeleteObject(bgBr);
    }

    BitBlt(dc,0,0,cr.right,cr.bottom,mem,0,0,SRCCOPY);
    SelectObject(mem,oldb); DeleteObject(bmp); DeleteDC(mem);
    EndPaint(hw,&ps);
}

static void UstawKomunikat(HWND hw, const wchar_t* txt, bool ok) {
    g_komunikat=txt?txt:L""; g_komunikatOK=ok;
    InvalidateRect(hw,nullptr,FALSE);
}

// ─── Fabryki kontrolek ───────────────────────────────────────────────────────
static void UsunKontrolePanelowe(HWND hw) {
    std::vector<HWND> v;
    EnumChildWindows(hw,[](HWND c,LPARAM lp)->BOOL{
        if (GetDlgCtrlID(c)>=200) {
            ((std::vector<HWND>*)lp)->push_back(c);
        }
        return TRUE;
    },(LPARAM)&v);
    for (HWND h:v) DestroyWindow(h);
}
static void UsunWszystkie(HWND hw) {
    std::vector<HWND> v;
    EnumChildWindows(hw,[](HWND c,LPARAM lp)->BOOL{
        ((std::vector<HWND>*)lp)->push_back(c); return TRUE;
    },(LPARAM)&v);
    for (HWND h:v) DestroyWindow(h);
}

static HWND MkStatic(HWND p,const wchar_t* t,int x,int y,int w,int h){
    HWND hw=CreateWindowEx(0,L"STATIC",t,WS_CHILD|WS_VISIBLE|SS_LEFT,x,y,w,h,p,nullptr,nullptr,nullptr);
    SendMessage(hw,WM_SETFONT,(WPARAM)g_fSmall,TRUE); return hw;
}
static HWND MkEdit(HWND p,int id,int x,int y,int w,int h,bool pwd=false){
    DWORD s=WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL|(pwd?ES_PASSWORD:0);
    HWND hw=CreateWindowEx(WS_EX_CLIENTEDGE,L"EDIT",L"",s,x,y,w,h,p,(HMENU)(size_t)id,nullptr,nullptr);
    SendMessage(hw,WM_SETFONT,(WPARAM)g_fBody,TRUE);
    SendMessage(hw,EM_SETREADONLY,TRUE,0); return hw;
}
static HWND MkBtn(HWND p,const wchar_t* t,int id,int x,int y,int w,int h,HFONT f=nullptr){
    HWND hw=CreateWindowEx(0,L"BUTTON",t,WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,
        x,y,w,h,p,(HMENU)(size_t)id,nullptr,nullptr);
    SendMessage(hw,WM_SETFONT,(WPARAM)(f?f:g_fBody),TRUE); return hw;
}
static HWND MkRadio(HWND p,const wchar_t* t,int id,int x,int y,int w,int h,bool first=false){
    DWORD s=WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTORADIOBUTTON|(first?WS_GROUP:0);
    HWND hw=CreateWindowEx(0,L"BUTTON",t,s,x,y,w,h,p,(HMENU)(size_t)id,nullptr,nullptr);
    SendMessage(hw,WM_SETFONT,(WPARAM)g_fSmall,TRUE); return hw;
}

// Klawiatura numeryczna 3x4 (1-9, del, 0, okID)
static void UtworzKlawiature(HWND par,int bx,int by,int okID,const wchar_t* okLbl=L"Dalej ->"){
    const int KW=68,KH=42,GX=8,GY=8;
    for (int i=1;i<=9;i++){
        int col=(i-1)%3, row=(i-1)/3;
        wchar_t lbl[4]; swprintf(lbl,4,L"%d",i);
        MkBtn(par,lbl,IDC_NUM_BASE+i, bx+col*(KW+GX),by+row*(KH+GY),KW,KH);
    }
    MkBtn(par,L"<--",IDC_NUM_BASE+10, bx+0*(KW+GX),by+3*(KH+GY),KW,KH);
    MkBtn(par,L"0",  IDC_NUM_BASE+0,  bx+1*(KW+GX),by+3*(KH+GY),KW,KH);
    MkBtn(par,okLbl, okID,            bx+2*(KW+GX),by+3*(KH+GY),KW,KH);
}

static void OdswiezEdit(HWND hw,int eid,const std::wstring& v){
    HWND he=GetDlgItem(hw,eid); if(he) SetWindowText(he,v.c_str());
}
static bool ObsluzNumpad(HWND hw,int id,std::wstring& buf,int eid,size_t mx=12){
    if (id>=IDC_NUM_BASE && id<=IDC_NUM_BASE+9){
        if (buf.size()<mx) buf+=(wchar_t)(L'0'+id-IDC_NUM_BASE);
        OdswiezEdit(hw,eid,buf); return true;
    }
    if (id==IDC_NUM_BASE+10){ if(!buf.empty()) buf.pop_back(); OdswiezEdit(hw,eid,buf); return true; }
    return false;
}

// ─── Widoki ──────────────────────────────────────────────────────────────────

static void ShowGlowny(HWND hw){
    UsunWszystkie(hw); g_widok=Widok::GLOWNY;
    RECT cr; GetClientRect(hw,&cr);
    int bw=160,bh=44, by=cr.bottom-bh-16;
    int cx=(cr.right-(bw*2+12))/2;

    if (g_rola != Rola::SERWISANT) {
        MkBtn(hw,L"Odbierz paczke",IDC_BTN_ODBIOR, cx,       by,bw,bh);
        MkBtn(hw,L"Nadaj paczke",  IDC_BTN_NADANIE,cx+bw+12, by,bw,bh);
    }

    if (g_rola==Rola::BRAK) {
        int ry=by-50;
        MkBtn(hw,L"Kurier [2222]",    IDC_BTN_KURIER,   cx,       ry,bw,36,g_fSmall);
        MkBtn(hw,L"Serwisant [1111]", IDC_BTN_SERWISANT,cx+bw+12,ry,bw,36,g_fSmall);
    } else {
        MkBtn(hw,L"Wyloguj",IDC_BTN_WYLOGUJ, cr.right-100,18,88,26,g_fSmall);
        if (g_rola==Rola::SERWISANT) {
            // przycisk resetu – klikniecie na kafelek ustawia g_podswietlona
            MkBtn(hw,L"Reset zaznaczonej skrytki",IDC_SRV_RESET, cx,by-50,bw*2+12,36,g_fSmall);
        }
    }
    OdbudujKafle(hw); InvalidateRect(hw,nullptr,FALSE);
}

static void ShowLogowanie(HWND hw, Rola r){
    UsunKontrolePanelowe(hw);
    g_logowanieDoRoli=r; g_bufKod.clear();
    g_widok=(r==Rola::KURIER)?Widok::LOGOWANIE_KURIER:Widok::LOGOWANIE_SERWISANT;
    RECT cr; GetClientRect(hw,&cr);
    int px=20,pw=cr.right-40,py=155;
    const wchar_t* tyt=(r==Rola::KURIER)?L"Logowanie - Kurier (PIN 2222)":L"Logowanie - Serwisant (PIN 1111)";
    MkStatic(hw,tyt,px,py,pw,22);
    MkStatic(hw,L"Wprowadz PIN:",px,py+32,pw,18);
    MkEdit(hw,IDC_LOG_PIN,px,py+54,pw,30,true);
    int kx=px+(pw-(3*68+2*8))/2;
    UtworzKlawiature(hw,kx,py+96,IDC_LOG_OK,L"Zaloguj");
    MkBtn(hw,L"Anuluj",IDC_LOG_CANCEL, px,py+96+4*(42+8)+8,pw,36);
}

static void ShowOdbiorTel(HWND hw){
    UsunKontrolePanelowe(hw); g_bufTel.clear(); g_widok=Widok::ODBIOR_TEL;
    RECT cr; GetClientRect(hw,&cr); int px=20,pw=cr.right-40,py=155;
    MkStatic(hw,L"Odbior paczki - krok 1/2: Numer telefonu",px,py,pw,22);
    MkEdit(hw,IDC_OD_TEL,px,py+32,pw,30);
    int kx=px+(pw-(3*68+2*8))/2;
    UtworzKlawiature(hw,kx,py+72,IDC_OD_OK,L"Dalej ->");
    MkBtn(hw,L"<- Anuluj",IDC_OD_BACK, px,py+72+4*(42+8)+8,pw,36);
}

static void ShowOdbiorKod(HWND hw){
    UsunKontrolePanelowe(hw); g_bufKod.clear(); g_widok=Widok::ODBIOR_KOD;
    RECT cr; GetClientRect(hw,&cr); int px=20,pw=cr.right-40,py=155;
    wchar_t inf[64]; swprintf(inf,64,L"Odbior - krok 2/2  |  Tel: %s",g_odTel.c_str());
    MkStatic(hw,inf,px,py,pw,22);
    MkStatic(hw,L"Kod odbioru:",px,py+32,pw,18);
    MkEdit(hw,IDC_OD_KOD,px,py+54,pw,30);
    int kx=px+(pw-(3*68+2*8))/2;
    UtworzKlawiature(hw,kx,py+96,IDC_OD_OK,L"Odbierz");
    MkBtn(hw,L"<- Wstecz",IDC_OD_BACK, px,py+96+4*(42+8)+8,pw,36);
}

static void ShowNadanieTel(HWND hw){
    UsunKontrolePanelowe(hw); g_bufTel.clear(); g_widok=Widok::NADANIE_TEL;
    RECT cr; GetClientRect(hw,&cr); int px=20,pw=cr.right-40,py=155;
    MkStatic(hw,L"Nadanie paczki - krok 1/3: Numer telefonu",px,py,pw,22);
    MkEdit(hw,IDC_NA_TEL,px,py+32,pw,30);
    int kx=px+(pw-(3*68+2*8))/2;
    UtworzKlawiature(hw,kx,py+72,IDC_NA_OK,L"Dalej ->");
    MkBtn(hw,L"<- Anuluj",IDC_NA_BACK, px,py+72+4*(42+8)+8,pw,36);
}

static void ShowNadanieKod(HWND hw){
    UsunKontrolePanelowe(hw); g_bufKod.clear(); g_widok=Widok::NADANIE_KOD;
    RECT cr; GetClientRect(hw,&cr); int px=20,pw=cr.right-40,py=155;
    wchar_t inf[64]; swprintf(inf,64,L"Nadanie - krok 2/3  |  Tel: %s",g_naTel.c_str());
    MkStatic(hw,inf,px,py,pw,22);
    MkStatic(hw,L"Kod odbioru (zapamietaj!):",px,py+32,pw,18);
    MkEdit(hw,IDC_NA_KOD,px,py+54,pw,30);
    int kx=px+(pw-(3*68+2*8))/2;
    UtworzKlawiature(hw,kx,py+96,IDC_NA_OK,L"Dalej ->");
    MkBtn(hw,L"<- Wstecz",IDC_NA_BACK, px,py+96+4*(42+8)+8,pw,36);
}

static void ShowNadanieRozmiar(HWND hw){
    UsunKontrolePanelowe(hw); g_rozmiar='A'; g_widok=Widok::NADANIE_ROZMIAR;
    RECT cr; GetClientRect(hw,&cr); int px=20,pw=cr.right-40,py=155;
    wchar_t inf[80]; swprintf(inf,80,L"Nadanie - krok 3/3  |  Tel: %s  Kod: %s",g_naTel.c_str(),g_naKod.c_str());
    MkStatic(hw,inf,px,py,pw,22);
    MkStatic(hw,L"Rozmiar paczki:",px,py+34,pw,18);
    int rw=(pw-16)/3;
    HWND ra=MkRadio(hw,L"A - Maly",  IDC_NA_RA,px,       py+56,rw,24,true);
    MkRadio(hw,L"B - Sredni",IDC_NA_RB,px+rw+8, py+56,rw,24);
    MkRadio(hw,L"C - Duzy",  IDC_NA_RC,px+2*(rw+8),py+56,rw,24);
    SendMessage(ra,BM_SETCHECK,BST_CHECKED,0);
    MkBtn(hw,L"Nadaj paczke",IDC_NA_OK,   px,       py+92,pw/2-6,44);
    MkBtn(hw,L"<- Wstecz",   IDC_NA_BACK, px+pw/2+6,py+92,pw/2-6,44);
}

static void ShowKurierTel(HWND hw){
    UsunKontrolePanelowe(hw); g_bufTel.clear(); g_widok=Widok::KURIER_TEL;
    RECT cr; GetClientRect(hw,&cr); int px=20,pw=cr.right-40,py=155;
    MkStatic(hw,L"Kurier - nadanie - krok 1/2: Numer telefonu odbiorcy",px,py,pw,22);
    MkEdit(hw,IDC_KUR_TEL,px,py+32,pw,30);
    int kx=px+(pw-(3*68+2*8))/2;
    UtworzKlawiature(hw,kx,py+72,IDC_KUR_OK,L"Dalej ->");
    MkBtn(hw,L"<- Anuluj",IDC_KUR_BACK, px,py+72+4*(42+8)+8,pw,36);
}

static void ShowKurierRozmiar(HWND hw){
    UsunKontrolePanelowe(hw); g_rozmiar='A'; g_widok=Widok::KURIER_ROZMIAR;
    RECT cr; GetClientRect(hw,&cr); int px=20,pw=cr.right-40,py=155;
    wchar_t inf[64]; swprintf(inf,64,L"Kurier - krok 2/2  |  Tel: %s",g_kurTel.c_str());
    MkStatic(hw,inf,px,py,pw,22);
    MkStatic(hw,L"Rozmiar paczki:",px,py+34,pw,18);
    int rw=(pw-16)/3;
    HWND ra=MkRadio(hw,L"A - Maly",  IDC_KUR_RA,px,        py+56,rw,24,true);
    MkRadio(hw,L"B - Sredni",IDC_KUR_RB,px+rw+8, py+56,rw,24);
    MkRadio(hw,L"C - Duzy",  IDC_KUR_RC,px+2*(rw+8),py+56,rw,24);
    SendMessage(ra,BM_SETCHECK,BST_CHECKED,0);
    MkBtn(hw,L"Nadaj (kurier)",IDC_KUR_OK,  px,       py+92,pw/2-6,44);
    MkBtn(hw,L"<- Wstecz",    IDC_KUR_BACK,px+pw/2+6,py+92,pw/2-6,44);
}

// Widok potwierdzenia zamkniecia skrytki (klient i kurier)
static std::wstring g_potwierdzenieInfo;  // tekst wyswietlany uzytkownikowi

static void ShowPotwierdzenie(HWND hw, const wchar_t* info) {
    UsunKontrolePanelowe(hw);
    g_potwierdzenieInfo = info;
    g_widok = Widok::POTWIERDZENIE;
    RECT cr; GetClientRect(hw,&cr);
    int px=20, pw=cr.right-40, py=170;

    // duzy napis z numerem skrytki (SS_LEFT | SS_NOPREFIX | wieloliniowy)
    HWND hInfo = CreateWindowEx(0, L"STATIC", info,
        WS_CHILD|WS_VISIBLE|SS_LEFT|SS_NOPREFIX,
        px, py+30, pw, 80, hw, nullptr, nullptr, nullptr);
    SendMessage(hInfo, WM_SETFONT, (WPARAM)g_fBody, TRUE);

    // instrukcja – z wystarczajacym odstepem pod tekstem info
    HWND hInstr = CreateWindowEx(0, L"STATIC",
        L"Wyjmij paczke, a nastepnie zamknij skrytke.",
        WS_CHILD|WS_VISIBLE|SS_LEFT|SS_NOPREFIX,
        px, py+90, pw, 22, hw, nullptr, nullptr, nullptr);
    SendMessage(hInstr, WM_SETFONT, (WPARAM)g_fSmall, TRUE);

    // JEDYNY przycisk – pod instrukcja
    MkBtn(hw, L"Zamknalem skrytke", IDC_POTWIERDZ,
          px + (pw-260)/2, py+130, 260, 54);
}

// Widok serwisanta = GLOWNY z kaflami trybu 2 – brak osobnego panelu
static void ShowSerwisant(HWND hw){ ShowGlowny(hw); }

// ─── WndProc ─────────────────────────────────────────────────────────────────
static LRESULT CALLBACK MainWndProc(HWND hw,UINT msg,WPARAM wp,LPARAM lp){
    static HBRUSH s_bgBr=nullptr;
    static HBRUSH s_panBr=nullptr;

    switch(msg){
    case WM_CREATE:
        s_bgBr =CreateSolidBrush(COL_BG);
        s_panBr=CreateSolidBrush(COL_PANEL);
        ShowGlowny(hw);
        return 0;

    case WM_SIZE:
        OdbudujKafle(hw);
        switch(g_widok){
            case Widok::GLOWNY:              ShowGlowny(hw); break;
            case Widok::ODBIOR_TEL:          ShowOdbiorTel(hw); break;
            case Widok::ODBIOR_KOD:          ShowOdbiorKod(hw); break;
            case Widok::NADANIE_TEL:         ShowNadanieTel(hw); break;
            case Widok::NADANIE_KOD:         ShowNadanieKod(hw); break;
            case Widok::NADANIE_ROZMIAR:     ShowNadanieRozmiar(hw); break;
            case Widok::LOGOWANIE_KURIER:
            case Widok::LOGOWANIE_SERWISANT: ShowLogowanie(hw,g_logowanieDoRoli); break;
            case Widok::KURIER_TEL:          ShowKurierTel(hw); break;
            case Widok::KURIER_ROZMIAR:      ShowKurierRozmiar(hw); break;
            case Widok::SERWISANT:           ShowSerwisant(hw); break;
            case Widok::POTWIERDZENIE:       ShowPotwierdzenie(hw,g_potwierdzenieInfo.c_str()); break;
        }
        InvalidateRect(hw,nullptr,FALSE);
        return 0;

    case WM_LBUTTONDOWN: {
        if (g_rola==Rola::SERWISANT && g_widok==Widok::GLOWNY) {
            POINT pt = { LOWORD(lp), HIWORD(lp) };
            for (const auto& k : g_kafle) {
                if (PtInRect(&k.rc, pt)) {
                    g_podswietlona = (g_podswietlona==k.id) ? -1 : k.id;
                    InvalidateRect(hw,nullptr,FALSE);
                    break;
                }
            }
        }
        return 0;
    }
    case WM_PAINT:   OnPaint(hw); return 0;
    case WM_ERASEBKGND: return 1;

    case WM_CTLCOLORBTN:
        SetBkColor((HDC)wp,COL_PANEL); SetTextColor((HDC)wp,COL_TEXT);
        return (LRESULT)s_panBr;
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
        SetBkColor((HDC)wp,COL_PANEL); SetTextColor((HDC)wp,COL_TEXT);
        return (LRESULT)s_panBr;

    case WM_COMMAND: {
        int id=LOWORD(wp);

        // ── przyciski rol na glownym ──────────────────────────────────
        if (id==IDC_BTN_KURIER)    { ShowLogowanie(hw,Rola::KURIER);    return 0; }
        if (id==IDC_BTN_SERWISANT && g_rola==Rola::BRAK) { ShowLogowanie(hw,Rola::SERWISANT); return 0; }
        if (id==IDC_BTN_WYLOGUJ)   { g_rola=Rola::BRAK; UstawKomunikat(hw,L"Wylogowano.",true); ShowGlowny(hw); return 0; }
        if (id==IDC_BTN_ODBIOR)    { g_podswietlona=-1; ShowOdbiorTel(hw); return 0; }
        if (id==IDC_BTN_NADANIE && g_rola==Rola::BRAK)   { g_podswietlona=-1; ShowNadanieTel(hw); return 0; }
        if (id==IDC_BTN_NADANIE && g_rola==Rola::KURIER)  { g_podswietlona=-1; ShowKurierTel(hw);  return 0; }

        // ── logowanie ─────────────────────────────────────────────────
        if (g_widok==Widok::LOGOWANIE_KURIER||g_widok==Widok::LOGOWANIE_SERWISANT){
            if (ObsluzNumpad(hw,id,g_bufKod,IDC_LOG_PIN,6)) return 0;
            if (id==IDC_LOG_OK){
                int pin=g_bufKod.empty()?0:_wtoi(g_bufKod.c_str());
                if (g_logowanieDoRoli==Rola::KURIER&&pin==PIN_KURIER){
                    g_rola=Rola::KURIER; UstawKomunikat(hw,L"Zalogowano jako Kurier.",true); ShowGlowny(hw);
                } else if (g_logowanieDoRoli==Rola::SERWISANT&&pin==PIN_SERWISANT){
                    g_rola=Rola::SERWISANT; UstawKomunikat(hw,L"Zalogowano jako Serwisant.",true); ShowGlowny(hw);
                } else {
                    UstawKomunikat(hw,L"Bledny PIN!",false); g_bufKod.clear(); OdswiezEdit(hw,IDC_LOG_PIN,L"");
                }
                return 0;
            }
            if (id==IDC_LOG_CANCEL){ ShowGlowny(hw); return 0; }
            return 0;
        }

        // ── odbior krok 1 ─────────────────────────────────────────────
        if (g_widok==Widok::ODBIOR_TEL){
            if (ObsluzNumpad(hw,id,g_bufTel,IDC_OD_TEL)) return 0;
            if (id==IDC_OD_OK){
                if (g_bufTel.size()<9){ UstawKomunikat(hw,L"Numer telefonu za krotki (min 9 cyfr).",false); return 0; }
                g_odTel=g_bufTel; UstawKomunikat(hw,nullptr,true); ShowOdbiorKod(hw);
                return 0;
            }
            if (id==IDC_OD_BACK){ ShowGlowny(hw); return 0; }
            return 0;
        }

        // ── odbior krok 2 ─────────────────────────────────────────────
        if (g_widok==Widok::ODBIOR_KOD){
            if (ObsluzNumpad(hw,id,g_bufKod,IDC_OD_KOD,6)) return 0;
            if (id==IDC_OD_OK){
                if (g_bufKod.size()<4){ UstawKomunikat(hw,L"Kod odbioru za krotki (min 4 cyfry).",false); return 0; }
                int kod=_wtoi(g_bufKod.c_str());
                // znajdz numer skrytki przed odebraniem (do podswietlenia)
                int nrSkrytki = -1;
                for (const Skrytka& s : g_pm->getSkrytki()) {
                    if (!s.czyZajeta()) continue;
                    const auto& p = s.getPaczka();
                    if (p.has_value() && p->getTelOdbiorcy()==FromWide(g_odTel) && p->getKodPaczki()==kod) {
                        nrSkrytki = s.getNumer(); break;
                    }
                }
                if (g_pm->odbiorGUI(FromWide(g_odTel),kod)){
                    g_podswietlona = nrSkrytki;
                    OdbudujKafle(hw);
                    wchar_t info[96];
                    swprintf(info,96,L"Skrytka nr %d jest otwarta.\nZabierz swoja paczke.",nrSkrytki);
                    UstawKomunikat(hw,nullptr,true);
                    ShowPotwierdzenie(hw,info);
                } else {
                    UstawKomunikat(hw,L"Nie znaleziono paczki - sprawdz tel i kod.",false);
                }
                return 0;
            }
            if (id==IDC_OD_BACK){ ShowOdbiorTel(hw); return 0; }
            return 0;
        }

        // ── nadanie krok 1 ────────────────────────────────────────────
        if (g_widok==Widok::NADANIE_TEL){
            if (ObsluzNumpad(hw,id,g_bufTel,IDC_NA_TEL)) return 0;
            if (id==IDC_NA_OK){
                if (g_bufTel.size()<9){ UstawKomunikat(hw,L"Numer telefonu za krotki (min 9 cyfr).",false); return 0; }
                g_naTel=g_bufTel; UstawKomunikat(hw,nullptr,true); ShowNadanieKod(hw);
                return 0;
            }
            if (id==IDC_NA_BACK){ ShowGlowny(hw); return 0; }
            return 0;
        }

        // ── nadanie krok 2 ────────────────────────────────────────────
        if (g_widok==Widok::NADANIE_KOD){
            if (ObsluzNumpad(hw,id,g_bufKod,IDC_NA_KOD,6)) return 0;
            if (id==IDC_NA_OK){
                if (g_bufKod.size()<4){ UstawKomunikat(hw,L"Kod odbioru za krotki (min 4 cyfry).",false); return 0; }
                g_naKod=g_bufKod; UstawKomunikat(hw,nullptr,true); ShowNadanieRozmiar(hw);
                return 0;
            }
            if (id==IDC_NA_BACK){ ShowNadanieTel(hw); return 0; }
            return 0;
        }

        // ── nadanie krok 3 ────────────────────────-───────────────────
        if (g_widok==Widok::NADANIE_ROZMIAR){
            if (id==IDC_NA_RA){g_rozmiar='A';return 0;}
            if (id==IDC_NA_RB){g_rozmiar='B';return 0;}
            if (id==IDC_NA_RC){g_rozmiar='C';return 0;}
            if (id==IDC_NA_OK){
                HWND ra=GetDlgItem(hw,IDC_NA_RA),rb=GetDlgItem(hw,IDC_NA_RB),rc=GetDlgItem(hw,IDC_NA_RC);
                if (ra&&SendMessage(ra,BM_GETCHECK,0,0)==BST_CHECKED) g_rozmiar='A';
                else if (rb&&SendMessage(rb,BM_GETCHECK,0,0)==BST_CHECKED) g_rozmiar='B';
                else if (rc&&SendMessage(rc,BM_GETCHECK,0,0)==BST_CHECKED) g_rozmiar='C';
                std::string tel=FromWide(g_naTel); int kod=_wtoi(g_naKod.c_str());
                if (g_pm->nadanieGUI(tel,kod,g_rozmiar)){
                    OdbudujKafle(hw);
                    // znajdz skrytke ktora wlasnie dostala paczke
                    int nrN = -1;
                    for (const Skrytka& s : g_pm->getSkrytki()) {
                        if (!s.czyZajeta()) continue;
                        const auto& p = s.getPaczka();
                        if (p.has_value() && p->getTelOdbiorcy()==tel && p->getKodPaczki()==kod) {
                            nrN = s.getNumer(); break;
                        }
                    }
                    wchar_t info[80];
                    swprintf(info,80,L"\n\nSkrytka nr %d jest otwarta.\nWloz paczke i zamknij.",nrN);
                    UstawKomunikat(hw,nullptr,true);
                    ShowPotwierdzenie(hw,info);
                } else {
                    // diagnostyka
                    bool tD=false,kD=false;
                    for (const Skrytka& s:g_pm->getSkrytki()){
                        if(!s.czyZajeta()) continue;
                        const auto& p=s.getPaczka(); if(!p.has_value()) continue;
                        if(p->getTelOdbiorcy()==tel) tD=true;
                        if(p->getKodPaczki()==kod)   kD=true;
                    }
                    if (tD)      UstawKomunikat(hw,L"Ten numer telefonu ma juz paczke w systemie!",false);
                    else if (kD) UstawKomunikat(hw,L"Ten kod odbioru jest juz zajety - wybierz inny.",false);
                    else         UstawKomunikat(hw,L"Brak wolnych skrytek odpowiedniego rozmiaru.",false);
                }
                return 0;
            }
            if (id==IDC_NA_BACK){ ShowNadanieKod(hw); return 0; }
            return 0;
        }

        // ── kurier krok 1 ─────────────────────────────────────────────
        if (g_widok==Widok::KURIER_TEL){
            if (ObsluzNumpad(hw,id,g_bufTel,IDC_KUR_TEL)) return 0;
            if (id==IDC_KUR_OK){
                if (g_bufTel.size()<9){ UstawKomunikat(hw,L"Numer telefonu za krotki (min 9 cyfr).",false); return 0; }
                g_kurTel=g_bufTel; UstawKomunikat(hw,nullptr,true); ShowKurierRozmiar(hw);
                return 0;
            }
            if (id==IDC_KUR_BACK){ ShowGlowny(hw); return 0; }
            return 0;
        }

        // ── kurier krok 2 ─────────────────────────────────────────────
        if (g_widok==Widok::KURIER_ROZMIAR){
            if (id==IDC_KUR_RA){g_rozmiar='A';return 0;}
            if (id==IDC_KUR_RB){g_rozmiar='B';return 0;}
            if (id==IDC_KUR_RC){g_rozmiar='C';return 0;}
            if (id==IDC_KUR_OK){
                HWND ra=GetDlgItem(hw,IDC_KUR_RA),rb=GetDlgItem(hw,IDC_KUR_RB),rc=GetDlgItem(hw,IDC_KUR_RC);
                if (ra&&SendMessage(ra,BM_GETCHECK,0,0)==BST_CHECKED) g_rozmiar='A';
                else if (rb&&SendMessage(rb,BM_GETCHECK,0,0)==BST_CHECKED) g_rozmiar='B';
                else if (rc&&SendMessage(rc,BM_GETCHECK,0,0)==BST_CHECKED) g_rozmiar='C';
                std::string tel=FromWide(g_kurTel);
                int genKod=g_pm->nadanieKurierGUI(tel,g_rozmiar);
                if (genKod>0){
                    OdbudujKafle(hw);
                    int nrK = -1;
                    for (const Skrytka& s : g_pm->getSkrytki()) {
                        if (!s.czyZajeta()) continue;
                        const auto& p = s.getPaczka();
                        if (p.has_value() && p->getTelOdbiorcy()==tel && p->getKodPaczki()==genKod) {
                            nrK = s.getNumer(); break;
                        }
                    }
                    wchar_t info[96];
                    swprintf(info,96,L"Skrytka nr %d jest otwarta.\nWloz paczke. Kod odbiorcy: %d",nrK,genKod);
                    UstawKomunikat(hw,nullptr,true);
                    ShowPotwierdzenie(hw,info);
                } else {
                    bool tD=false;
                    for (const Skrytka& s:g_pm->getSkrytki()){
                        if(!s.czyZajeta()) continue;
                        const auto& p=s.getPaczka();
                        if(p.has_value()&&p->getTelOdbiorcy()==tel){tD=true;break;}
                    }
                    UstawKomunikat(hw,tD?L"Ten numer telefonu ma juz paczke w systemie!":
                                       L"Brak wolnych skrytek odpowiedniego rozmiaru.",false);
                }
                return 0;
            }
            if (id==IDC_KUR_BACK){ ShowKurierTel(hw); return 0; }
            return 0;
        }

        // ── potwierdzenie zamkniecia skrytki ──────────────────────────
        if (g_widok==Widok::POTWIERDZENIE){
            if (id==IDC_POTWIERDZ){
                UstawKomunikat(hw,L"Dziekujemy!",true);
                g_podswietlona=-1;
                ShowGlowny(hw);
            }
            return 0;
        }

        // ── serwisant – reset zaznaczonej skrytki ────────────────────
        if (id==IDC_SRV_RESET){
            if (g_podswietlona < 0) {
                UstawKomunikat(hw,L"Kliknij na skrytke ktora chcesz zresetowac.",false);
                return 0;
            }
            if (g_pm->resetSkrytki(g_podswietlona)){
                wchar_t kom[64]; swprintf(kom,64,L"Skrytka %d zresetowana.",g_podswietlona);
                g_podswietlona=-1;
                UstawKomunikat(hw,kom,true);
                OdbudujKafle(hw); InvalidateRect(hw,nullptr,FALSE);
            } else {
                UstawKomunikat(hw,L"Skrytka jest juz wolna.",false);
                g_podswietlona=-1;
            }
            return 0;
        }

        return 0;
    } // WM_COMMAND

    case WM_DESTROY:
        DeleteObject(s_bgBr); DeleteObject(s_panBr);
        PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hw,msg,wp,lp);
}

// ─── WinMain ─────────────────────────────────────────────────────────────────
int WINAPI WinMain(HINSTANCE hInst,HINSTANCE,LPSTR,int nShow){
    INITCOMMONCONTROLSEX icc={sizeof(icc),ICC_LISTVIEW_CLASSES|ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&icc);

    g_fTitle=MakeFont(32,true, L"Segoe UI");
    g_fBody =MakeFont(16,false,L"Segoe UI");
    g_fSmall=MakeFont(13,false,L"Segoe UI");

    std::vector<Skrytka> skrytki={
        Skrytka(1,'A'),Skrytka(2,'A'),Skrytka(3,'A'),
        Skrytka(4,'B'),Skrytka(5,'B'),Skrytka(6,'B'),
        Skrytka(7,'C'),Skrytka(8,'C')
    };
    Paczkomat pm("ul. Przykladowa 1","KRK01",skrytki);
    // kilka paczek startowych
    pm.nadanieGUI("600100200",1234,'A');
    pm.nadanieGUI("500200300",5678,'B');
    pm.nadanieGUI("700300400",9012,'C');
    g_pm=&pm;

    WNDCLASSEX wc={};
    wc.cbSize=sizeof(wc); wc.style=CS_HREDRAW|CS_VREDRAW;
    wc.lpfnWndProc=MainWndProc; wc.hInstance=hInst;
    wc.hCursor=LoadCursor(nullptr,IDC_ARROW);
    wc.hbrBackground=CreateSolidBrush(COL_BG);
    wc.lpszClassName=L"PaczkomatWnd";
    wc.hIcon=LoadIcon(nullptr,IDI_APPLICATION);
    RegisterClassEx(&wc);

    HWND hWnd=CreateWindowEx(0,L"PaczkomatWnd",L"Paczkomat - KRK01",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,CW_USEDEFAULT,660,720,
        nullptr,nullptr,hInst,nullptr);

    ShowWindow(hWnd,nShow); UpdateWindow(hWnd);

    MSG m;
    while(GetMessage(&m,nullptr,0,0)){
        TranslateMessage(&m); DispatchMessage(&m);
    }
    DeleteObject(g_fTitle); DeleteObject(g_fBody); DeleteObject(g_fSmall);
    return (int)m.wParam;
}
