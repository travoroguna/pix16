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


int APIENTRY WinMain(HINSTANCE instance, HINSTANCE prev_inst, LPSTR argv, int argc)
{
    os_init();

    print("Hello, %S!", S("World"));

    return 0;
}
