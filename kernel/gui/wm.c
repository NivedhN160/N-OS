#include "wm.h"
#include "vga.h"
#include "rtc.h"
#include "icons.h"

Window windows[MAX_WINDOWS];
Window *top_window = 0;
int dragging_win_id = -1;
int drag_start_x = 0, drag_start_y = 0;
int drag_start_wx = 0, drag_start_wy = 0;

int start_menu_open = 0;
int current_desktop = 0;
int expose_mode = 0;
int battery_popup_open = 0;
int wifi_popup_open = 0;
int spotlight_active = 0;
char spotlight_buf[32];
int spotlight_len = 0;
char clipboard[5][32];

static void scopy(char *d,const char *s){int i=0;while(s[i]){d[i]=s[i];i++;}d[i]=0;}

void wm_init() {
    for (int i=0; i<MAX_WINDOWS; i++) windows[i].flags = 0;
    top_window = 0;
}

int wm_create_window(int x, int y, int w, int h, const char *title, window_draw_func on_draw) {
    for (int i=0; i<MAX_WINDOWS; i++) {
        if ((windows[i].flags & WIN_FLAG_SHOW) == 0) {
            windows[i].id = i;
            windows[i].x = x; windows[i].y = y;
            windows[i].w = w; windows[i].h = h;
            scopy(windows[i].title, title);
            windows[i].flags = WIN_FLAG_SHOW | WIN_FLAG_ACTIVE;
            windows[i].desktop = current_desktop;
            windows[i].on_draw = on_draw;
            windows[i].on_mouse = 0;
            windows[i].on_key = 0;
            windows[i].is_minimized = 0;
            windows[i].is_maximized = 0;
            windows[i].orig_x = x; windows[i].orig_y = y;
            windows[i].orig_w = w; windows[i].orig_h = h;
            
            windows[i].next = top_window;
            top_window = &windows[i];
            
            Window *curr = top_window->next;
            while(curr) { curr->flags &= ~WIN_FLAG_ACTIVE; curr = curr->next; }
            expose_mode = 0;
            return i;
        }
    }
    return -1;
}

void wm_set_callbacks(int id, window_mouse_func on_mouse, window_key_func on_key) {
    if (id >= 0 && id < MAX_WINDOWS) {
        windows[id].on_mouse = on_mouse;
        windows[id].on_key = on_key;
    }
}

void wm_close_window(int id) {
    if (id < 0 || id >= MAX_WINDOWS) return;
    windows[id].flags = 0;
    if (top_window == &windows[id]) {
        top_window = top_window->next;
        if (top_window) top_window->flags |= WIN_FLAG_ACTIVE;
    } else {
        Window *curr = top_window;
        while(curr && curr->next != &windows[id]) curr = curr->next;
        if (curr && curr->next == &windows[id]) curr->next = windows[id].next;
    }
}

void bring_to_front(Window *w) {
    if (top_window == w) return;
    if (top_window) top_window->flags &= ~WIN_FLAG_ACTIVE;
    w->flags |= WIN_FLAG_ACTIVE;
    Window *curr = top_window;
    while(curr && curr->next != w) curr = curr->next;
    if (curr && curr->next == w) curr->next = w->next;
    w->next = top_window;
    top_window = w;
}

void format_num2(char *buf, int n) { buf[0]=(n/10)+'0'; buf[1]=(n%10)+'0'; buf[2]=0; }

void draw_taskbar() {
    int tb_h = 40;
    int tb_y = SCREEN_HEIGHT - tb_h;
    
    blur_region_alpha(0, tb_y, SCREEN_WIDTH, tb_h, 6, 0x111122, 160); // Frosted Glass Taskbar
    
    // Start Button (Windows 11 style center, but let's keep it left for now)
    int tb_x = 10;
    
    // High-Def Start Button Icon (Custom Sphere Logo)
    draw_image(tb_x, tb_y + 8, 24, 24, icon_start);
    
    tb_x += 40;
    
    // Expose Button
    draw_rounded_rect_alpha(tb_x, tb_y + 8, 30, 24, 6, 0x444466, 180);
    draw_rect(tb_x + 8, tb_y + 12, 6, 6, 0xFFFFFF);
    draw_rect(tb_x + 16, tb_y + 12, 6, 6, 0xFFFFFF);
    draw_rect(tb_x + 8, tb_y + 20, 6, 6, 0xFFFFFF);
    draw_rect(tb_x + 16, tb_y + 20, 6, 6, 0xFFFFFF);
    
    // Windows on taskbar
    int w_x = tb_x + 50;
    Window *curr = top_window;
    while(curr) {
        if (curr->desktop == current_desktop) {
            unsigned int c = (curr->flags & WIN_FLAG_ACTIVE) ? 0x555577 : 0x333344;
            draw_rounded_rect_alpha(w_x, tb_y + 8, 120, 24, 5, c, 180);
            
            // Draw small app icon based on title
            if (curr->title[0] == 'T' && curr->title[1] == 'e') draw_image(w_x+4, tb_y+12, 16, 16, icon_terminal);
            else if (curr->title[0] == 'F' && curr->title[1] == 'i') draw_image(w_x+4, tb_y+12, 16, 16, icon_firefox);
            else if (curr->title[0] == 'N' && curr->title[1] == 'o') draw_image(w_x+4, tb_y+12, 16, 16, icon_notepad);
            else if (curr->title[0] == 'C' && curr->title[1] == 'a') draw_image(w_x+4, tb_y+12, 16, 16, icon_calc);
            else draw_image(w_x+4, tb_y+12, 16, 16, icon_folder);
            
            char short_title[12];
            int idx = 0;
            while(curr->title[idx] && idx < 10) { short_title[idx] = curr->title[idx]; idx++; }
            short_title[idx] = 0;
            draw_string(w_x + 24, tb_y + 16, short_title, 0xFFFFFF);
            w_x += 130;
        }
        curr = curr->next;
    }
    
    // System Tray (Time, Battery, Wi-Fi)
    time_t t; rtc_read_time(&t);
    char hstr[3], mstr[3]; format_num2(hstr, t.hour); format_num2(mstr, t.minute);
    
    int tray_x = SCREEN_WIDTH - 200;
    
    // BT Icon
    draw_rounded_rect_alpha(tray_x, tb_y+12, 16, 16, 4, 0x0055FF, 255);
    draw_string(tray_x+4, tb_y+16, "B", 0xFFFFFF);
    
    tray_x += 25;
    // Wi-Fi Icon (Arcs)
    draw_rounded_rect_alpha(tray_x, tb_y+12, 16, 16, 4, 0x333333, 200);
    draw_rect_alpha(tray_x+4, tb_y+16, 8, 2, 0x00FF00, 255);
    draw_rect_alpha(tray_x+6, tb_y+20, 4, 2, 0x00FF00, 255);
    draw_rect_alpha(tray_x+7, tb_y+24, 2, 2, 0x00FF00, 255);
    
    tray_x += 25;
    // Battery Icon
    draw_rounded_rect_alpha(tray_x, tb_y+12, 20, 12, 2, 0xFFFFFF, 200); // outline
    draw_rect(tray_x+2, tb_y+14, 16, 8, 0x00FF00); // 100% full
    draw_rect(tray_x+20, tb_y+15, 2, 6, 0xFFFFFF); // battery tip
    
    tray_x += 35;
    // Time
    draw_string(tray_x, tb_y + 16, hstr, 0xFFFFFF);
    draw_string(tray_x + 16, tb_y + 16, ":", 0xFFFFFF);
    draw_string(tray_x + 24, tb_y + 16, mstr, 0xFFFFFF);
    
    // Popups
    if (battery_popup_open) {
        draw_rounded_rect_alpha(SCREEN_WIDTH - 180, tb_y - 60, 170, 50, 5, 0x222233, 230);
        draw_string(SCREEN_WIDTH - 170, tb_y - 50, "Battery: 100%", 0xFFFFFF);
        draw_string(SCREEN_WIDTH - 170, tb_y - 35, "Status: Fully Charged", 0xAAAAAA);
    }
    if (wifi_popup_open) {
        draw_rounded_rect_alpha(SCREEN_WIDTH - 180, tb_y - 80, 170, 70, 5, 0x111122, 230);
        draw_string(SCREEN_WIDTH - 170, tb_y - 70, "Wi-Fi: Connected", 0xFFFFFF);
        draw_string(SCREEN_WIDTH - 170, tb_y - 55, "Network: N-OS_Net", 0x00FF00);
        draw_string(SCREEN_WIDTH - 170, tb_y - 40, "IP: 192.168.1.100", 0xAAAAAA);
    }
}

void draw_start_menu() {
    if (!start_menu_open) return;
    int tb_h = 40;
    int sm_w = 200, sm_h = 360;
    int sm_x = 10;
    int sm_y = SCREEN_HEIGHT - tb_h - sm_h - 10;
    
    draw_shadow_rect(sm_x, sm_y, sm_w, sm_h);
    blur_region_alpha(sm_x, sm_y, sm_w, sm_h, 8, 0x1A1C29, 180); // Frosted Glass
    
    // Header
    draw_string(sm_x + 10, sm_y + 10, "N-OS Applications", 0xFFFFFF);
    draw_rect_alpha(sm_x + 10, sm_y + 25, sm_w - 20, 1, 0x555577, 255);
    
    // Items with Icons
    int y_off = 35;
    
    draw_image(sm_x + 10, sm_y + y_off, 16, 16, icon_terminal);
    draw_string(sm_x + 35, sm_y + y_off + 4, "Terminal", 0xFFFFFF);
    y_off += 25;
    
    draw_image(sm_x + 10, sm_y + y_off, 16, 16, icon_folder);
    draw_string(sm_x + 35, sm_y + y_off + 4, "File Manager", 0xFFFFFF);
    y_off += 25;
    
    draw_image(sm_x + 10, sm_y + y_off, 16, 16, icon_settings);
    draw_string(sm_x + 35, sm_y + y_off + 4, "Task Manager", 0xFFFFFF);
    y_off += 25;
    
    extern int app_installed_firefox;
    extern int app_installed_calc;
    
    if (app_installed_calc) {
        draw_image(sm_x + 10, sm_y + y_off, 16, 16, icon_calc);
        draw_string(sm_x + 35, sm_y + y_off + 4, "Calculator", 0xFFFFFF);
        y_off += 25;
    }
    
    draw_image(sm_x + 10, sm_y + y_off, 16, 16, icon_notepad);
    draw_string(sm_x + 35, sm_y + y_off + 4, "Notepad", 0xFFFFFF);
    y_off += 25;
    
    draw_image(sm_x + 10, sm_y + y_off, 16, 16, icon_settings);
    draw_string(sm_x + 35, sm_y + y_off + 4, "Settings", 0xFFFFFF);
    y_off += 25;
    
    if (app_installed_firefox) {
        draw_image(sm_x + 10, sm_y + y_off, 16, 16, icon_firefox);
        draw_string(sm_x + 35, sm_y + y_off + 4, "Firefox Browser", 0xFFFFFF);
        y_off += 25;
    }
    
    draw_image(sm_x + 10, sm_y + y_off, 16, 16, icon_settings); // Just using settings icon for now
    draw_string(sm_x + 35, sm_y + y_off + 4, "App Store", 0xFFFFFF);
    y_off += 35;
    
    // System Options at the bottom
    draw_rect_alpha(sm_x + 10, sm_y + y_off, sm_w - 20, 1, 0x555577, 255);
    y_off += 10;
    
    draw_string(sm_x + 10, sm_y + y_off, "Lock", 0xFFA500);
    draw_string(sm_x + 100, sm_y + y_off, "Sleep", 0x0000FF);
    y_off += 20;
    
    draw_string(sm_x + 10, sm_y + y_off, "Reboot", 0xFF0000);
    draw_string(sm_x + 100, sm_y + y_off, "Shutdown", 0xFF0000);
}

void draw_spotlight() {
    if (!spotlight_active) return;
    int sx = (SCREEN_WIDTH - 400) / 2;
    int sy = 100;
    draw_shadow_rect(sx, sy, 400, 60);
    draw_rounded_rect_alpha(sx, sy, 400, 60, 10, 0xFFFFFF, 230);
    draw_string(sx+20, sy+15, "Spotlight Search", 0x444444);
    draw_rect_alpha(sx+20, sy+35, 360, 16, 0xDDDDDD, 255);
    draw_string(sx+22, sy+39, spotlight_buf, 0x000000);
}

int wallpaper_type = 0; // 0=Gradient, 1=Solid Dark, 2=Solid Blue

void wm_render() {
    extern int gpu_accelerated;
    // Beautiful abstract high-res wallpaper
    if (wallpaper_type == 0) {
        for (int y=0; y<SCREEN_HEIGHT; y++) {
            if (gpu_accelerated) {
                // Peach Graphics
                unsigned int r = 255;
                unsigned int g = 200 - ((y * 100) / SCREEN_HEIGHT);
                unsigned int b = 150 - ((y * 100) / SCREEN_HEIGHT);
                draw_rect(0, y, SCREEN_WIDTH, 1, (r << 16) | (g << 8) | b);
            } else {
                unsigned int r = (y * 255) / SCREEN_HEIGHT;
                unsigned int g = 100;
                unsigned int b = 255 - ((y * 100) / SCREEN_HEIGHT);
                draw_rect(0, y, SCREEN_WIDTH, 1, (r << 16) | (g << 8) | b);
            }
        }
    } else if (wallpaper_type == 1) {
        clear_screen(0x111122);
    } else if (wallpaper_type == 2) {
        clear_screen(0x004488);
    } else if (wallpaper_type == 3) {
        clear_screen(0xFFFFFF); // TempleOS white
        for(int x=0; x<SCREEN_WIDTH; x+=40) draw_rect(x, 0, 1, SCREEN_HEIGHT, 0x0000FF);
        for(int y=0; y<SCREEN_HEIGHT; y+=40) draw_rect(0, y, SCREEN_WIDTH, 1, 0x0000FF);
    }
    
    Window *stack[MAX_WINDOWS];
    int count = 0;
    Window *curr = top_window;
    while(curr) { stack[count++] = curr; curr = curr->next; }
    
    if (expose_mode) {
        int ex = 50, ey = 50;
        for (int i=count-1; i>=0; i--) {
            Window *w = stack[i];
            draw_shadow_rect(ex, ey, 200, 150);
            draw_rounded_rect_alpha(ex, ey, 200, 150, 10, 0xFFFFFF, 200);
            draw_rect_alpha(ex, ey, 200, 20, 0x000000, 100);
            draw_string(ex+10, ey+6, w->title, 0xFFFFFF);
            ex += 250;
            if (ex > SCREEN_WIDTH - 250) { ex = 50; ey += 200; }
        }
    } else {
        for (int i=count-1; i>=0; i--) {
            Window *w = stack[i];
            if (w->desktop != current_desktop || w->is_minimized) continue;
            
            draw_shadow_rect(w->x, w->y, w->w, w->h);
            // Glass window body
            if (dragging_win_id == w->id) {
                draw_rect_alpha(w->x, w->y, w->w, w->h, 0xEEEEFF, 200);
            } else {
                blur_region_alpha(w->x, w->y, w->w, w->h, 6, 0xEEEEFF, 200);
            }
            
            // Title bar
            unsigned int tc1 = (w->flags & WIN_FLAG_ACTIVE) ? 0xDDDDDD : 0xEEEEEE;
            unsigned int tc2 = (w->flags & WIN_FLAG_ACTIVE) ? 0xAAAAAA : 0xCCCCCC;
            draw_rounded_rect_alpha(w->x, w->y, w->w, 24, 0, tc2, 255);
            
            draw_string(w->x + 70, w->y + 8, w->title, 0x000000); // Shifted text
            
            // Mac-style traffic light buttons
            draw_rounded_rect_alpha(w->x+10, w->y+6, 12, 12, 6, 0xFF5F56, 255); // Red (Close)
            draw_rounded_rect_alpha(w->x+28, w->y+6, 12, 12, 6, 0xFFBD2E, 255); // Yellow (Minimize)
            draw_rounded_rect_alpha(w->x+46, w->y+6, 12, 12, 6, 0x27C93F, 255); // Green (Maximize)
            
            if (w->on_draw) w->on_draw(w->x, w->y+24, w->w, w->h-24);
        }
    }
    
    draw_taskbar();
    draw_start_menu();
    draw_spotlight();
}

int wm_handle_mouse(int mx, int my, int mb, int dx, int dy) {
    int tb_h = 40;
    int tb_y = SCREEN_HEIGHT - tb_h;
    int tb_w = SCREEN_WIDTH;
    int tb_x = 0;
    
    extern int current_cursor_type;
    current_cursor_type = 0; // Default arrow
    
    // Hover logic for Taskbar
    if (my >= tb_y) {
        if (mx >= tb_x && mx <= tb_x+50) current_cursor_type = 1; // Start
        if (mx >= SCREEN_WIDTH - 200 && mx <= SCREEN_WIDTH - 150) current_cursor_type = 1; // Wi-Fi / BT
        if (mx >= SCREEN_WIDTH - 120 && mx <= SCREEN_WIDTH - 80) current_cursor_type = 1; // Battery
    }
    
    // Hover logic for Window controls
    Window *cw = top_window;
    while(cw) {
        if (cw->desktop == current_desktop && !cw->is_minimized) {
            if (mx >= cw->x && mx < cw->x + cw->w && my >= cw->y && my < cw->y + cw->h) {
                if (my < cw->y + 24 && mx >= cw->x + 10 && mx <= cw->x + 58) current_cursor_type = 1; // Mac buttons
                break;
            }
        }
        cw = cw->next;
    }
    
    static int is_dragging = 0;
    
    // Handle Window Dragging & Snap Tiling
    if (dragging_win_id != -1) {
        if (mb & 1) {
            Window *w = &windows[dragging_win_id];
            w->x = drag_start_wx + (mx - drag_start_x);
            w->y = drag_start_wy + (my - drag_start_y);
            
            // Snap Tiling visual preview would go here
            return 0;
        } else {
            Window *w = &windows[dragging_win_id];
            // On release, check snap tiling
            if (mx <= 5) {
                w->orig_x = w->x; w->orig_y = w->y; w->orig_w = w->w; w->orig_h = w->h;
                w->x = 0; w->y = 0; w->w = SCREEN_WIDTH/2; w->h = SCREEN_HEIGHT - tb_h;
                w->is_maximized = 1;
            } else if (mx >= SCREEN_WIDTH - 5) {
                w->orig_x = w->x; w->orig_y = w->y; w->orig_w = w->w; w->orig_h = w->h;
                w->x = SCREEN_WIDTH/2; w->y = 0; w->w = SCREEN_WIDTH/2; w->h = SCREEN_HEIGHT - tb_h;
                w->is_maximized = 1;
            } else if (my <= 5) {
                w->orig_x = w->x; w->orig_y = w->y; w->orig_w = w->w; w->orig_h = w->h;
                w->x = 0; w->y = 0; w->w = SCREEN_WIDTH; w->h = SCREEN_HEIGHT - tb_h;
                w->is_maximized = 1;
            }
            dragging_win_id = -1;
        }
    }
    
    if (mb == 0) return 0; // Just hovering
    
    if (mb & 1) {
        // Taskbar clicks
        if (mx >= tb_x && mx <= tb_x+tb_w && my >= tb_y && my <= tb_y+tb_h) {
            if (mx >= tb_x+10 && mx <= tb_x+50) start_menu_open = !start_menu_open;
            else if (mx >= tb_x+60 && mx <= tb_x+90) expose_mode = !expose_mode;
            else if (mx >= SCREEN_WIDTH - 150 && mx <= SCREEN_WIDTH - 130) wifi_popup_open = !wifi_popup_open;
            else if (mx >= SCREEN_WIDTH - 120 && mx <= SCREEN_WIDTH - 80) battery_popup_open = !battery_popup_open;
            else {
                int w_x = tb_x + 50;
                Window *curr = top_window;
                int clicked_tab = 0;
                while(curr) {
                    if (curr->desktop == current_desktop) {
                        if (mx >= w_x && mx <= w_x + 120) {
                            if (curr->is_minimized) {
                                curr->is_minimized = 0;
                                bring_to_front(curr);
                            } else {
                                if (curr == top_window) {
                                    curr->is_minimized = 1;
                                } else {
                                    bring_to_front(curr);
                                }
                            }
                            clicked_tab = 1;
                            break;
                        }
                        w_x += 130;
                    }
                    curr = curr->next;
                }
                if (!clicked_tab) {
                    start_menu_open = 0; battery_popup_open = 0; wifi_popup_open = 0;
                }
            }
            return 0;
        }

        if (start_menu_open) {
            int sm_w = 200, sm_h = 360, sm_x = 10, sm_y = SCREEN_HEIGHT - 40 - sm_h - 10;
            if (mx >= sm_x && mx <= sm_x+sm_w && my >= sm_y && my <= sm_y+sm_h) {
                start_menu_open = 0;
                int ry = my - sm_y;
                
                // We must match the draw_start_menu Y offsets dynamically
                extern int app_installed_firefox;
                extern int app_installed_calc;
                
                int y_off = 35;
                if (ry >= y_off && ry <= y_off+20) return 1; // Terminal
                y_off += 25;
                if (ry >= y_off && ry <= y_off+20) return 2; // File Manager
                y_off += 25;
                if (ry >= y_off && ry <= y_off+20) return 3; // Task Manager
                y_off += 25;
                
                if (app_installed_calc) {
                    if (ry >= y_off && ry <= y_off+20) return 5; // Calc
                    y_off += 25;
                }
                
                if (ry >= y_off && ry <= y_off+20) return 6; // Notepad
                y_off += 25;
                
                if (ry >= y_off && ry <= y_off+20) return 12; // Settings
                y_off += 25;
                
                if (app_installed_firefox) {
                    if (ry >= y_off && ry <= y_off+20) return 11; // Firefox
                    y_off += 25;
                }
                
                if (ry >= y_off && ry <= y_off+20) return 13; // App Store
                y_off += 35;
                
                y_off += 10;
                if (ry >= y_off && ry <= y_off+15 && mx < sm_x+90) return 9; // Lock
                if (ry >= y_off && ry <= y_off+15 && mx >= sm_x+90) return 10; // Sleep
                y_off += 20;
                
                if (ry >= y_off && ry <= y_off+15 && mx < sm_x+90) return 8; // Reboot
                if (ry >= y_off && ry <= y_off+15 && mx >= sm_x+90) return 4; // Shutdown
                return 0;
            }
            start_menu_open = 0;
            battery_popup_open = 0;
            wifi_popup_open = 0;
        }
        
        Window *curr = top_window;
        while(curr) {
            if (curr->desktop == current_desktop && 
                mx >= curr->x && mx < curr->x + curr->w &&
                my >= curr->y && my < curr->y + curr->h) {
                bring_to_front(curr);
                if (my < curr->y + 24) {
                    if (mx >= curr->x + 10 && mx <= curr->x + 22) { // Close
                        wm_close_window(curr->id);
                        return 0;
                    }
                    if (mx >= curr->x + 28 && mx <= curr->x + 40) { // Minimize
                        curr->is_minimized = 1;
                        return 0;
                    }
                    if (mx >= curr->x + 46 && mx <= curr->x + 58) { // Maximize
                        if (curr->is_maximized) {
                            curr->x = curr->orig_x; curr->y = curr->orig_y;
                            curr->w = curr->orig_w; curr->h = curr->orig_h;
                            curr->is_maximized = 0;
                        } else {
                            curr->orig_x = curr->x; curr->orig_y = curr->y;
                            curr->orig_w = curr->w; curr->orig_h = curr->h;
                            curr->x = 0; curr->y = 0;
                            curr->w = SCREEN_WIDTH; curr->h = SCREEN_HEIGHT - tb_h;
                            curr->is_maximized = 1;
                        }
                        return 0;
                    }
                    dragging_win_id = curr->id;
                    drag_start_x = mx; drag_start_y = my;
                    drag_start_wx = curr->x; drag_start_wy = curr->y;
                    return 0;
                }
                if (curr->on_mouse) curr->on_mouse(mx - curr->x, my - (curr->y + 24), mb);
                return 0;
            }
            curr = curr->next;
        }
    }
    return 0;
}

void wm_handle_key(char c) {
    if (spotlight_active) {
        if (c == '\n') spotlight_active = 0;
        else if (c == '\b' && spotlight_len > 0) spotlight_buf[--spotlight_len] = 0;
        else if (c && c != '\t' && spotlight_len < 31) {
            spotlight_buf[spotlight_len++] = c;
            spotlight_buf[spotlight_len] = 0;
        }
        return;
    }
    if (c == '/') {
        spotlight_active = 1;
        spotlight_len = 0; spotlight_buf[0]=0;
        return;
    }
    if (top_window && top_window->on_key && top_window->desktop == current_desktop) {
        top_window->on_key(c);
    }
}
