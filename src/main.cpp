#include <windows.h>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <string>
#include <array>
#include "sim.hpp"

static Simulator* g_sim = nullptr;
static int g_scale = 3;
static int g_brush = 5;

// --- Simple UI panel state ---
static bool g_uiCollapsed = false;
static const int UI_WIDTH = 140; // device pixels (stretched with window)

struct UiItem { const wchar_t* label; Element element; };
static std::array<UiItem,4> g_items{{
    {L"Sand (1)",  Element::Sand},
    {L"Water (2)", Element::Water},
    {L"Stone (3)", Element::Stone},
    {L"Eraser (0)",Element::Empty}
}};

static void blitToWindow(HDC hdc, RECT rc, const Simulator& sim){
    int sw = sim.width();
    int sh = sim.height();
    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = sw;
    bmi.bmiHeader.biHeight = -sh; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    const void* pixels = sim.frame().data();
    int dw = (rc.right - rc.left);
    int dh = (rc.bottom - rc.top);
    // Draw sim area: leave UI panel space if expanded
    int panelW = g_uiCollapsed ? 24 : UI_WIDTH;
    int simWpx = max(1, dw - panelW);
    StretchDIBits(hdc,
        0,0,simWpx,dh,
        0,0,sw,sh,
        pixels, &bmi, DIB_RGB_COLORS, SRCCOPY);

    // UI overlay
    RECT p = { simWpx, 0, dw, dh };
    HBRUSH bg = CreateSolidBrush(RGB(24,24,28));
    FillRect(hdc, &p, bg);
    DeleteObject(bg);

    // Collapse/expand header
    RECT hdr = { simWpx, 0, dw, 28 };
    HBRUSH hdrBrush = CreateSolidBrush(RGB(40,40,46));
    FillRect(hdc, &hdr, hdrBrush);
    DeleteObject(hdrBrush);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(220,220,230));
    DrawText(hdc, g_uiCollapsed ? L"»" : L"Elements «", -1, &hdr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    if(!g_uiCollapsed){
        int y = 36;
        for(const auto& it : g_items){
            RECT r{ simWpx+8, y, dw-8, y+28 };
            HBRUSH itemBg = CreateSolidBrush(RGB(55,55,62));
            FillRect(hdc, &r, itemBg);
            DeleteObject(itemBg);
            DrawText(hdc, it.label, -1, &r, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            y += 34;
        }
    }
}

static POINT clientMousePos(HWND hWnd){
    POINT p; GetCursorPos(&p); ScreenToClient(hWnd, &p); return p;
}

static bool handleUiClick(HWND hWnd, WPARAM wParam){
    RECT rc; GetClientRect(hWnd, &rc);
    int dw = rc.right - rc.left;
    int dh = rc.bottom - rc.top;
    int panelW = g_uiCollapsed ? 24 : UI_WIDTH;
    int simWpx = max(1, dw - panelW);
    POINT p = clientMousePos(hWnd);

    // Header toggle
    RECT hdr = { simWpx, 0, dw, 28 };
    if(PtInRect(&hdr, p)){
        g_uiCollapsed = !g_uiCollapsed;
        InvalidateRect(hWnd, nullptr, FALSE);
        return true;
    }
    if(g_uiCollapsed) return p.x >= simWpx; // clicks on collapsed panel do nothing else

    // Items
    int y = 36;
    for(const auto& it : g_items){
        RECT r{ simWpx+8, y, dw-8, y+28 };
        if(PtInRect(&r, p)){
            g_sim->setSelected(it.element);
            InvalidateRect(hWnd, nullptr, FALSE);
            return true;
        }
        y += 34;
    }
    // If click was inside the panel but not on an item, consume it.
    if(p.x >= simWpx) return true;
    return false;
}

static void paintAtMouse(HWND hWnd, WPARAM wParam){
    if(!g_sim) return;
    RECT rc; GetClientRect(hWnd, &rc);
    int dw = rc.right - rc.left;
    int dh = rc.bottom - rc.top;
    int panelW = g_uiCollapsed ? 24 : UI_WIDTH;
    int simWpx = max(1, dw - panelW);

    POINT p = clientMousePos(hWnd);
    if(p.x >= simWpx){
        // Click inside the UI panel — handled elsewhere.
        return;
    }

    int sx = p.x * g_sim->width()  / std::max(1, simWpx);
    int sy = p.y * g_sim->height() / std::max(1, dh);

    Element e = Element::Empty;
    bool allowOverwrite = false;
    if(wParam & MK_LBUTTON){
        e = g_sim->selected();
        // By default do NOT overwrite existing cells. (Bug #1 fix)
    } else if (wParam & MK_RBUTTON){
        e = Element::Empty; // eraser
        allowOverwrite = true; // allow clearing regardless of prior content
    } else {
        return;
    }
    g_sim->paint(sx, sy, g_brush, e, allowOverwrite);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam){
    switch(msg){
        case WM_CREATE:
            SetTimer(hWnd, 1, 16, nullptr); // ~60 Hz
            return 0;
        case WM_TIMER:
            if(g_sim){
                g_sim->tick();
                InvalidateRect(hWnd, nullptr, FALSE);
            }
            return 0;
        case WM_MOUSEMOVE:
            if(wParam & (MK_LBUTTON | MK_RBUTTON)){
                paintAtMouse(hWnd, wParam);
                InvalidateRect(hWnd, nullptr, FALSE);
            }
            return 0;
        case WM_LBUTTONDOWN: {
            // First, see if the UI wants the click
            RECT rc; GetClientRect(hWnd, &rc);
            int dw = rc.right - rc.left;
            int panelW = g_uiCollapsed ? 24 : UI_WIDTH;
            int simWpx = max(1, dw - panelW);
            POINT p = clientMousePos(hWnd);
            if(p.x >= simWpx){
                if(handleUiClick(hWnd, wParam)){
                    return 0;
                }
            }
            paintAtMouse(hWnd, wParam);
            SetCapture(hWnd);
            return 0;
        }
        case WM_RBUTTONDOWN:
            paintAtMouse(hWnd, wParam);
            SetCapture(hWnd);
            return 0;
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
            ReleaseCapture();
            return 0;
        case WM_MOUSEWHEEL: {
            short delta = GET_WHEEL_DELTA_WPARAM(wParam);
            if(delta > 0) g_brush = std::min(100, g_brush + 1);
            else g_brush = std::max(1, g_brush - 1);
            return 0;
        }
        case WM_KEYDOWN:
            switch(wParam){
                case '1': g_sim->setSelected(Element::Sand); break;
                case '2': g_sim->setSelected(Element::Water); break;
                case '3': g_sim->setSelected(Element::Stone); break;
                case '0': g_sim->setSelected(Element::Empty); break;
                case 'R': g_sim->reset(); break;
                case VK_ESCAPE: PostQuitMessage(0); break;
            }
            return 0;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            blitToWindow(hdc, ps.rcPaint, *g_sim);
            EndPaint(hWnd, &ps);
            return 0;
        }
        case WM_SIZE:
            InvalidateRect(hWnd, nullptr, FALSE);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow){
    SimConfig cfg; // defaults
    Simulator sim(cfg);
    g_sim = &sim;

    const wchar_t CLASS_NAME[] = L"PowderGameWindow";
    WNDCLASS wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);

    int winW = cfg.width * g_scale + (UI_WIDTH);
    int winH = cfg.height * g_scale;
    RECT r{0,0, winW, winH};
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    HWND hWnd = CreateWindowEx(
        0, CLASS_NAME, L"Powder Game (C++ Minimal)",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        r.right - r.left, r.bottom - r.top,
        nullptr, nullptr, hInstance, nullptr);

    ShowWindow(hWnd, nCmdShow);

    MSG msg;
    while(GetMessage(&msg, nullptr, 0, 0)){
        TranslateMessage(&msg);
        DispatchMessage(&msg, 0);
    }
    return 0;
}
