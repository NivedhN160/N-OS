// ── Win32 Compatibility Layer for N-OS
// Implements the foundational mock APIs for User32 and Kernel32

extern int wm_create_window(int x, int y, int w, int h, const char *title, void (*on_draw)(int,int,int,int));
extern void term_print(const char *s, unsigned int col);

// Mock HWND
typedef int HWND;

// Dummy draw function for Win32 apps
void win32_dummy_draw(int wx, int wy, int ww, int wh) {
    extern void draw_rect(int x, int y, int w, int h, unsigned int c);
    extern void draw_string(int x, int y, const char* str, unsigned int color);
    
    draw_rect(wx, wy, ww, wh, 0xFFFFFF); // White window background
    draw_string(wx + 10, wy + 20, "Running Real .exe via N-OS Win32 Layer!", 0x000000);
    draw_string(wx + 10, wy + 40, "Intercepted CreateWindowExA.", 0x000000);
}

// User32.dll: CreateWindowExA
HWND CreateWindowExA(
    unsigned int dwExStyle,
    const char *lpClassName,
    const char *lpWindowName,
    unsigned int dwStyle,
    int X, int Y,
    int nWidth, int nHeight,
    HWND hWndParent,
    int hMenu,
    int hInstance,
    void *lpParam
) {
    term_print("WIN32: Intercepted CreateWindowExA!\n", 0x00FF00);
    term_print("WIN32: Bridging to N-OS Graphics...\n", 0x00FF00);
    
    // Bridge the Windows API call directly to our OS's Window Manager!
    return wm_create_window(X, Y, nWidth, nHeight, lpWindowName, win32_dummy_draw);
}
