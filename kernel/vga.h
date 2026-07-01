#ifndef VGA_H
#define VGA_H

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768

extern unsigned int gfx_backbuffer[SCREEN_WIDTH * SCREEN_HEIGHT];

void gfx_init(unsigned int fb_addr, unsigned int pitch);
void draw_pixel(int x, int y, unsigned int color);
void draw_pixel_alpha(int x, int y, unsigned int color, unsigned char alpha);
void draw_rect(int x, int y, int w, int h, unsigned int color);
void draw_rect_alpha(int x, int y, int w, int h, unsigned int color, unsigned char alpha);
void clear_screen(unsigned int color);
void gfx_swap();
void draw_char(int x, int y, char c, unsigned int color);
void draw_string(int x, int y, const char *s, unsigned int color);
void draw_char_scaled(int x, int y, char c, unsigned int color, int scale);
void draw_string_scaled(int x, int y, const char *s, unsigned int color, int scale);

void blur_region_alpha(int x, int y, int w, int h, int radius, unsigned int tint_color, unsigned char tint_alpha);

void draw_shadow_rect(int x, int y, int w, int h);
void draw_rounded_rect_alpha(int x, int y, int w, int h, int radius, unsigned int color, unsigned char alpha);
void draw_image(int x, int y, int w, int h, const unsigned int *data);

// Dirty rect mouse
void save_mouse_bg(int mx, int my);
void restore_mouse_bg(int mx, int my);
void draw_mouse(int mx, int my);

#endif
