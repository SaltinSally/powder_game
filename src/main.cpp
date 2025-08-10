#include <windows.h>
#include <cstdint>
#include <vector>
#include <chrono>
#include "sim.hpp"
#include <algorithm>


static Simulator* g_sim = nullptr;
static int g_scale = 3;
static int g_brush = 5;

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
    StretchDIBits(hdc,
        0,0,dw,dh,
        0,0,sw,sh,
        pixels, &bmi, DIB_RGB_COLORS, SRCCOPY);
}

static POINT clientMousePos(HWND hWnd){
    POINT p; GetCursorPos(&p); ScreenToClient(hWnd, &p); return p;
}

static void paintAtMouse(HWND hWnd, WPARAM wParam){
    if(!g_sim) return;
    POINT p = clientMousePos(hWnd);
    RECT rc; GetClientRect(hWnd, &rc);
    int dw = rc.right - rc.left;
    int dh = rc.bottom - rc.top;
    int sx = p.x * g_sim->width()  / std::max(1, dw);
    int sy = p.y * g_sim->height() / std::max(1, dh);

    Element e = Element::Empty;
    if(wParam & MK_LBUTTON){
        e = g_sim->selected();
    } else if (wParam & MK_RBUTTON){
        e = Element::Empty;
    } else {
        return;
    }
    g_sim->paint(sx, sy, g_brush, e);
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
        case WM_LBUTTONDOWN:
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
            if (delta > 0) g_brush = std::min(100, g_brush + 1);
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

    int winW = cfg.width * g_scale;
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
        DispatchMessage(&msg);
    }
    return 0;
}
