#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOMINMAX
#include <windows.h>

//
// NOTE(nick): set up base library code
//

#define STB_SPRINTF_IMPLEMENTATION
#include "third_party/stb_sprintf.h"

static char *win32__print_callback(const char *buf, void *user, int len) {
    DWORD bytes_written;
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    WriteFile(handle, buf, len, &bytes_written, 0);
    return (char *)buf;
}

static void win32__print(const char *format, ...) {
    char buffer[1024];

    va_list args;
    va_start(args, format);
    stbsp_vsprintfcb(win32__print_callback, 0, buffer, format, args);
    va_end(args);
}

#define PrintToBuffer stbsp_vsnprintf
#define print win32__print

#define impl
#include "third_party/na.h"
#include "third_party/na_math.h"

//
// NOTE(nick): win32 entry code
//

#pragma comment(lib, "gdi32")
#pragma comment(lib, "shell32")
#pragma comment(lib, "advapi32")

#ifndef WINDOW_CLASS_NAME
    #define WINDOW_CLASS_NAME L"Pix16 Window Class"
#endif

struct Win32_Framebuffer
{
    i32 width;
    i32 height;
    i32 bytes_per_pixel;
    u32 *pixels;
    
    BITMAPINFO bitmap_info;
    HBITMAP bitmap;
};

//
// NOTE(nick): win32 globals
//
global b32 win32_window_is_fullscreen = false;
global Win32_Framebuffer win32_framebuffer = {0};

function void
win32_toggle_fullscreen(HWND hwnd)
{
    static WINDOWPLACEMENT placement = {0};

    b32 is_fullscreen = false;
    {
        MONITORINFO monitor_info = {0};
        monitor_info.cbSize = sizeof(MONITORINFO);
        GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &monitor_info);

        RECT rect;
        GetWindowRect(hwnd, &rect);

        is_fullscreen = (
            rect.left == monitor_info.rcMonitor.left
            && rect.right == monitor_info.rcMonitor.right
            && rect.top == monitor_info.rcMonitor.top
            && rect.bottom == monitor_info.rcMonitor.bottom
        );
    }

    DWORD style = GetWindowLong(hwnd, GWL_STYLE);

    if (!is_fullscreen)
    {
        MONITORINFO monitor_info = {sizeof(monitor_info)};

        if (
            GetWindowPlacement(hwnd, &placement) &&
            GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &monitor_info)
        ) {
            SetWindowLong(hwnd, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);

            SetWindowPos(
                hwnd,
                HWND_TOP,
                monitor_info.rcMonitor.left,
                monitor_info.rcMonitor.top,
                monitor_info.rcMonitor.right  - monitor_info.rcMonitor.left,
                monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
                SWP_NOOWNERZORDER | SWP_FRAMECHANGED
            );
        }
    } else {
        SetWindowLong(hwnd, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(hwnd, &placement);
        DWORD flags = SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED;
        SetWindowPos(hwnd, 0, 0, 0, 0, 0, flags);
    }
}

function b32
win32_resize_framebuffer(Win32_Framebuffer *it, int width, int height)
{
    assert(it);
    assert(width > 0 && height > 0);

    if (it->width == width && it->height == height)
    {
        return false;
    }

    BITMAPINFO *info = &it->bitmap_info;
    info->bmiHeader.biSize        = sizeof(info->bmiHeader);
    info->bmiHeader.biWidth       = width;
    info->bmiHeader.biHeight      = -height; // NOTE(bill): -ve is top-down, +ve is bottom-up
    info->bmiHeader.biPlanes      = 1;
    info->bmiHeader.biBitCount    = 32;
    info->bmiHeader.biCompression = BI_RGB;

    if (it->pixels)
    {
        VirtualFree(it->pixels, 0, MEM_RELEASE);
    }

    it->width = width;
    it->height = height;
    it->bytes_per_pixel = 4;

    i64 size = (it->width * it->height) * it->bytes_per_pixel;
    it->pixels = (u32 *)VirtualAlloc(0, size, MEM_COMMIT, PAGE_READWRITE);

    return true;
}


function LRESULT CALLBACK
win32_window_callback(HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param)
{
    switch (message)
    {
        case WM_PAINT:
        {
            RECT wr;
            GetClientRect(hwnd, &wr);
            int width = (int)(wr.right - wr.left);
            int height = (int)(wr.bottom - wr.top);

            // NOTE(nick): this will invalidate any drawing that has happened in the window!
            win32_resize_framebuffer(&win32_framebuffer, width, height);

            return DefWindowProcW(hwnd, message, w_param, l_param);
        } break;

        case WM_DPICHANGED:
        {
            // Resize windowed mode windows that either permit rescaling or that
            // need it to compensate for non-client area scaling
            RECT *suggested = cast(RECT *)l_param;
            SetWindowPos(hwnd, HWND_TOP,
                            suggested->left,
                            suggested->top,
                            suggested->right - suggested->left,
                            suggested->bottom - suggested->top,
                            SWP_NOACTIVATE | SWP_NOZORDER);
        } break;

        case WM_KEYDOWN:
        {
            b32 was_down = !!(l_param & (1 << 30));
            b32 is_down  =  !(l_param & (1 << 31));

            if (!was_down && is_down)
            {
                if (w_param == VK_F11)
                {
                    win32_toggle_fullscreen(hwnd);
                }
            }

        } break;

        default: {} break;
    }

    return DefWindowProcW(hwnd, message, w_param, l_param);
}

function void
win32_fatal_assert(b32 cond, char *text) {
    if (!cond)
    {
        MessageBoxA(NULL, text, "Pix16: Fatal Error", MB_ICONERROR | MB_OK);
        os_exit(1);
    }
}

int APIENTRY WinMain(HINSTANCE instance, HINSTANCE prev_inst, LPSTR argv, int argc)
{
    os_init();

    // NOTE(nick): Set DPI Awareness
    HMODULE user32 = LoadLibraryA("user32.dll");

    typedef BOOL Win32_SetProcessDpiAwarenessContext(HANDLE);
    typedef BOOL Win32_SetProcessDpiAwareness(int);

    Win32_SetProcessDpiAwarenessContext *SetProcessDpiAwarenessContext =
        (Win32_SetProcessDpiAwarenessContext *) GetProcAddress(user32, "SetProcessDpiAwarenessContext");
    Win32_SetProcessDpiAwareness *SetProcessDpiAwareness =
        (Win32_SetProcessDpiAwareness *) GetProcAddress(user32, "SetProcessDpiAwareness");

    if (SetProcessDpiAwarenessContext) {
        SetProcessDpiAwarenessContext(((HANDLE) -4) /* DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 */);
    } else if (SetProcessDpiAwareness) {
        SetProcessDpiAwareness(1 /* PROCESS_SYSTEM_DPI_AWARE */);
    } else {
        SetProcessDPIAware();
    }

    // NOTE(nick): set up global window class

    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc   = (WNDPROC)win32_window_callback;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = instance;
    wc.hIcon         = LoadIconA(instance, "APPICON");
    wc.hCursor       = LoadCursorA(0, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = WINDOW_CLASS_NAME;
    wc.hIconSm       = NULL;

    win32_fatal_assert(RegisterClassExW(&wc), "Failed to register window class.");

    print("Hello, %S!", S("World"));

    M_Temp scratch = GetScratch(0, 0);

    u32 scale = 2;
    Vector2i size = v2i(scale * 320, scale * 240);
    String16 title16 = string16_from_string(scratch.arena, S("Pix16"));

    HWND hwnd = CreateWindowW(
        WINDOW_CLASS_NAME,
        (WCHAR *)title16.data, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, size.x, size.y,
        0, 0, instance, 0
    );

    HDC hdc = GetDC(hwnd);

    int window_width = size.x;
    int window_height = size.y;

    // NOTE(nick): scale the window by DPI (if applicable)
    {
        f32 scale = (f32)GetDeviceCaps(hdc, LOGPIXELSX) / (f32)USER_DEFAULT_SCREEN_DPI;
        if (scale > 1)
        {
            RECT wr = {0, 0, (LONG)(size.x * scale), (LONG)(size.y * scale)};
            AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
            window_width = (i32)(wr.right - wr.left);
            window_height = (i32)(wr.bottom - wr.top);

            SetWindowPos(hwnd, HWND_TOP, 0, 0, window_width, window_height, SWP_NOMOVE | SWP_NOOWNERZORDER);
        }
    }

    // NOTE(nick): center window
    {
        HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);

        MONITORINFO info = {};
        info.cbSize = sizeof(MONITORINFO);
        GetMonitorInfo(monitor, &info);
        int monitor_width = info.rcMonitor.right - info.rcMonitor.left;
        int monitor_height = info.rcMonitor.bottom - info.rcMonitor.top;

        int center_x = (monitor_width - window_width) / 2;
        int center_y = (monitor_height - window_height) / 2;

        SetWindowPos(hwnd, HWND_TOP, center_x, center_y, window_width, window_height, SWP_NOOWNERZORDER);
    }

    ShowWindow(hwnd, SW_SHOW);

    while (true)
    {
        // NOTE(nick): poll for events
        for (;;) {
            MSG msg = {};

            if (!PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
                break;
            }

            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        Win32_Framebuffer *framebuffer = &win32_framebuffer;
        for (i32 y = 0; y < framebuffer->height; y += 1)
        {
            for (i32 x = 0; x < framebuffer->width; x += 1)
            {
                framebuffer->pixels[y * framebuffer->width + x] = 0xffff0000;
            }
        }

        //
        // NOTE(nick): present framebuffer
        // @Incomplete: should we double-buffer this?
        //
        // @WTF: what is actually happening with StretchDIBits?
        // Why does it not work the way we think it should?
        // Meaning, I need to fill it every frame, how would I skip rendering for a frame with it?
        //
        {
            RECT wr = {0};
            GetClientRect(hwnd, &wr);
            int width = (int)(wr.right - wr.left);
            int height = (int)(wr.bottom - wr.top);

            Win32_Framebuffer *framebuffer = &win32_framebuffer;

            StretchDIBits(hdc,
                0, 0, width, height,
                0, 0, framebuffer->width, framebuffer->height, framebuffer->pixels,
                &framebuffer->bitmap_info,
                DIB_RGB_COLORS, SRCCOPY);

            SwapBuffers(hdc);
        }

        // @Incomplete: actually do proper frame-rate timing
        os_sleep(16 / 1000.0);
    }

    return 0;
}
