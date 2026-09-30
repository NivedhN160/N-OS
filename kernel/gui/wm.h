#ifndef WM_H
#define WM_H

#define MAX_WINDOWS 8
#define WIN_FLAG_ACTIVE 1
#define WIN_FLAG_SHOW   2

typedef void (*window_draw_func)(int wx, int wy, int ww, int wh);
typedef void (*window_mouse_func)(int rx, int ry, int mb);
typedef void (*window_key_func)(char c);

typedef struct Window {
    int id;
    int x, y, w, h;
    char title[32];
    int flags;
    int desktop; // For virtual desktops
    window_draw_func on_draw;
    window_mouse_func on_mouse;
    window_key_func on_key;
    int is_minimized;
    int is_maximized;
    int orig_x, orig_y, orig_w, orig_h;
    struct Window *next;
} Window;

extern Window windows[MAX_WINDOWS];
extern int current_desktop;
extern int expose_mode;
extern int spotlight_active;
extern char spotlight_buf[32];
extern int spotlight_len;
extern int wallpaper_type;
extern char clipboard[5][32];
extern int start_menu_open;
extern int dragged_window;
extern int drag_off_x, drag_off_y;

void wm_init();
int wm_create_window(int x, int y, int w, int h, const char *title, window_draw_func on_draw);
void wm_set_callbacks(int id, window_mouse_func on_mouse, window_key_func on_key);
void wm_close_window(int id);
void wm_render();
int wm_handle_mouse(int mx, int my, int mb, int dx, int dy);
void wm_handle_key(char c);

#endif
