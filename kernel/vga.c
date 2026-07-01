#include "vga.h"

unsigned int *gfx_framebuffer = 0;
unsigned int gfx_pitch = 0;

unsigned int gfx_backbuffer[SCREEN_WIDTH * SCREEN_HEIGHT];

unsigned int mouse_bg[16*16];
int mouse_saved_x = 0;
int mouse_saved_y = 0;

void gfx_init(unsigned int fb_addr, unsigned int pitch) {
    gfx_framebuffer = (unsigned int*)fb_addr;
    gfx_pitch = pitch;
}

void draw_pixel(int x, int y, unsigned int color) {
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
    gfx_backbuffer[y * SCREEN_WIDTH + x] = color;
}

void draw_pixel_alpha(int x, int y, unsigned int color, unsigned char alpha) {
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
    if (alpha == 255) { gfx_backbuffer[y * SCREEN_WIDTH + x] = color; return; }
    if (alpha == 0) return;
    
    unsigned int bg = gfx_backbuffer[y * SCREEN_WIDTH + x];
    
    unsigned int rb1 = bg & 0x00FF00FF;
    unsigned int g1  = bg & 0x0000FF00;
    unsigned int rb2 = color & 0x00FF00FF;
    unsigned int g2  = color & 0x0000FF00;
    
    unsigned int rb_out = ((rb1 * (255 - alpha) + rb2 * alpha) >> 8) & 0x00FF00FF;
    unsigned int g_out  = ((g1 * (255 - alpha) + g2 * alpha) >> 8) & 0x0000FF00;
    
    gfx_backbuffer[y * SCREEN_WIDTH + x] = rb_out | g_out;
}

void draw_rect(int x, int y, int w, int h, unsigned int color) {
    for (int j = y; j < y + h; j++)
        for (int i = x; i < x + w; i++)
            draw_pixel(i, j, color);
}

void draw_rect_alpha(int x, int y, int w, int h, unsigned int color, unsigned char alpha) {
    for (int j = y; j < y + h; j++)
        for (int i = x; i < x + w; i++)
            draw_pixel_alpha(i, j, color, alpha);
}

void draw_shadow_rect(int x, int y, int w, int h) {
    draw_rect_alpha(x+10, y+10, w, h, 0x000000, 100); // 10px soft shadow
}

void draw_rounded_rect_alpha(int x, int y, int w, int h, int r, unsigned int color, unsigned char alpha) {
    for (int j = y; j < y + h; j++) {
        for (int i = x; i < x + w; i++) {
            // Check corners
            if (i < x+r && j < y+r) {
                if ((i-(x+r))*(i-(x+r)) + (j-(y+r))*(j-(y+r)) > r*r) continue;
            } else if (i >= x+w-r && j < y+r) {
                if ((i-(x+w-r-1))*(i-(x+w-r-1)) + (j-(y+r))*(j-(y+r)) > r*r) continue;
            } else if (i < x+r && j >= y+h-r) {
                if ((i-(x+r))*(i-(x+r)) + (j-(y+h-r-1))*(j-(y+h-r-1)) > r*r) continue;
            } else if (i >= x+w-r && j >= y+h-r) {
                if ((i-(x+w-r-1))*(i-(x+w-r-1)) + (j-(y+h-r-1))*(j-(y+h-r-1)) > r*r) continue;
            }
            draw_pixel_alpha(i, j, color, alpha);
        }
    }
}

void clear_screen(unsigned int color) {
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++)
        gfx_backbuffer[i] = color;
}

void gfx_swap() {
    if(!gfx_framebuffer) return;
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        gfx_framebuffer[i] = gfx_backbuffer[i];
    }
}

void draw_image(int x, int y, int w, int h, const unsigned int *data) {
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            unsigned int color = data[j * w + i];
            // Treat magic pink 0xFF00FF or 0x00000000 alpha as transparent
            if ((color & 0xFFFFFF) != 0xFF00FF && ((color >> 24) != 0)) {
                draw_pixel_alpha(x + i, y + j, color & 0xFFFFFF, (color >> 24) & 0xFF);
            }
        }
    }
}

static unsigned char font[96][8] = {
    {0,0,0,0,0,0,0,0}, {0x18,0x18,0x18,0x18,0,0,0x18,0}, {0x6C,0x6C,0x6C,0,0,0,0,0}, {0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0}, {0x18,0x3E,0x60,0x3C,0x06,0x7C,0x18,0}, {0,0x66,0x6C,0x18,0x30,0x66,0x46,0}, {0x1C,0x36,0x1C,0x38,0x6F,0x66,0x3B,0}, {0x06,0x0C,0x18,0,0,0,0,0}, {0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0}, {0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0}, {0,0x66,0x3C,0xFF,0x3C,0x66,0,0}, {0,0x18,0x18,0x7E,0x18,0x18,0,0}, {0,0,0,0,0,0x18,0x18,0x30}, {0,0,0,0x7E,0,0,0,0}, {0,0,0,0,0,0x18,0x18,0}, {0x06,0x0C,0x18,0x30,0x60,0,0,0}, {0x3C,0x66,0x6E,0x76,0x66,0x66,0x3C,0}, {0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0}, {0x3C,0x66,0x06,0x1C,0x30,0x60,0x7E,0}, {0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0}, {0x0E,0x1E,0x36,0x66,0x7F,0x06,0x06,0}, {0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0}, {0x1C,0x30,0x60,0x7C,0x66,0x66,0x3C,0}, {0x7E,0x06,0x0C,0x18,0x30,0x30,0x30,0}, {0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0}, {0x3C,0x66,0x66,0x3E,0x06,0x0C,0x38,0}, {0,0x18,0x18,0,0x18,0x18,0,0}, {0,0x18,0x18,0,0x18,0x18,0x30,0}, {0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0}, {0,0,0x7E,0,0x7E,0,0,0}, {0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0}, {0x3C,0x66,0x06,0x0C,0x18,0,0x18,0}, {0x3E,0x63,0x6F,0x69,0x6F,0x60,0x3E,0}, {0x18,0x3C,0x66,0x7E,0x66,0x66,0x66,0}, {0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0}, {0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0}, {0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0}, {0x7E,0x60,0x60,0x78,0x60,0x60,0x7E,0}, {0x7E,0x60,0x60,0x78,0x60,0x60,0x60,0}, {0x3C,0x66,0x60,0x6E,0x66,0x66,0x3C,0}, {0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0}, {0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0}, {0x1E,0x0C,0x0C,0x0C,0x0C,0x6C,0x38,0}, {0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0}, {0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0}, {0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0}, {0x66,0x76,0x7E,0x7E,0x6E,0x66,0x66,0}, {0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0}, {0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0}, {0x3C,0x66,0x66,0x66,0x6E,0x3C,0x06,0}, {0x7C,0x66,0x66,0x7C,0x6C,0x66,0x66,0}, {0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0}, {0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0}, {0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0}, {0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0}, {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0}, {0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0}, {0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0}, {0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0}, {0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0}, {0x60,0x30,0x18,0x0C,0x06,0,0,0}, {0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0}, {0x18,0x3C,0x66,0,0,0,0,0}, {0,0,0,0,0,0,0,0xFF}, {0x18,0x18,0x0C,0,0,0,0,0}, {0,0,0x3C,0x06,0x3E,0x66,0x3E,0}, {0x60,0x60,0x7C,0x66,0x66,0x66,0x7C,0}, {0,0,0x3C,0x66,0x60,0x66,0x3C,0}, {0x06,0x06,0x3E,0x66,0x66,0x66,0x3E,0}, {0,0,0x3C,0x66,0x7E,0x60,0x3C,0}, {0x1C,0x30,0x7C,0x30,0x30,0x30,0x30,0}, {0,0,0x3E,0x66,0x66,0x3E,0x06,0x3C}, {0x60,0x60,0x7C,0x66,0x66,0x66,0x66,0}, {0x18,0,0x38,0x18,0x18,0x18,0x3C,0}, {0x06,0,0x06,0x06,0x06,0x66,0x3C,0}, {0x60,0x60,0x66,0x6C,0x78,0x6C,0x66,0}, {0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0}, {0,0,0x66,0x7F,0x7F,0x6B,0x63,0}, {0,0,0x7C,0x66,0x66,0x66,0x66,0}, {0,0,0x3C,0x66,0x66,0x66,0x3C,0}, {0,0,0x7C,0x66,0x66,0x7C,0x60,0x60}, {0,0,0x3E,0x66,0x66,0x3E,0x06,0x06}, {0,0,0x6C,0x76,0x60,0x60,0x60,0}, {0,0,0x3C,0x60,0x3C,0x06,0x7C,0}, {0x30,0x30,0x7C,0x30,0x30,0x30,0x1C,0}, {0,0,0x66,0x66,0x66,0x66,0x3E,0}, {0,0,0x66,0x66,0x66,0x3C,0x18,0}, {0,0,0x63,0x6B,0x7F,0x3E,0x36,0}, {0,0,0x66,0x3C,0x18,0x3C,0x66,0}, {0,0,0x66,0x66,0x3E,0x06,0x3C,0}, {0,0,0x7E,0x0C,0x18,0x30,0x7E,0}, {0x0C,0x18,0x18,0x70,0x18,0x18,0x0C,0}, {0x18,0x18,0x18,0,0x18,0x18,0x18,0}, {0x30,0x18,0x18,0x0E,0x18,0x18,0x30,0}, {0x38,0x6C,0x0C,0,0,0,0,0}, {0,0,0,0,0,0,0,0}
    };
    
void draw_char(int x, int y, char c, unsigned int color) {
    if (c < 32 || c > 127) return;
    unsigned char *glyph = font[c - 32];
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (glyph[row] & (0x80 >> col))
                draw_pixel(x + col, y + row, color);
        }
    }
}
void draw_gradient_rect_alpha(int x, int y, int w, int h, unsigned int top_color, unsigned int bot_color, unsigned char alpha) {
    if(!gfx_framebuffer) return;
    
    unsigned int tr = (top_color >> 16) & 0xFF, tg = (top_color >> 8) & 0xFF, tb = top_color & 0xFF;
    unsigned int br = (bot_color >> 16) & 0xFF, bg = (bot_color >> 8) & 0xFF, bb = bot_color & 0xFF;
    
    for (int r = 0; r < h; r++) {
        unsigned int cur_r = tr + ((br - tr) * r) / h;
        unsigned int cur_g = tg + ((bg - tg) * r) / h;
        unsigned int cur_b = tb + ((bb - tb) * r) / h;
        unsigned int c = (cur_r << 16) | (cur_g << 8) | cur_b;
        
        for (int c_idx = 0; c_idx < w; c_idx++) {
            if (x + c_idx < 0 || x + c_idx >= SCREEN_WIDTH || y + r < 0 || y + r >= SCREEN_HEIGHT) continue;
            unsigned int bg_col = gfx_framebuffer[(y+r)*SCREEN_WIDTH + (x+c_idx)];
            unsigned int b_r = (bg_col >> 16) & 0xFF, b_g = (bg_col >> 8) & 0xFF, b_b = bg_col & 0xFF;
            unsigned int out_r = (cur_r * alpha + b_r * (255 - alpha)) / 255;
            unsigned int out_g = (cur_g * alpha + b_g * (255 - alpha)) / 255;
            unsigned int out_b = (cur_b * alpha + b_b * (255 - alpha)) / 255;
            gfx_framebuffer[(y+r)*SCREEN_WIDTH + (x+c_idx)] = (out_r << 16) | (out_g << 8) | out_b;
        }
    }
    
    // RTX specular highlight (top and left white glow)
    for (int i=0; i<w; i++) if (y>=0 && y<SCREEN_HEIGHT && x+i>=0 && x+i<SCREEN_WIDTH) gfx_framebuffer[y*SCREEN_WIDTH + (x+i)] = 0xFFFFFF;
    for (int i=0; i<h; i++) if (y+i>=0 && y+i<SCREEN_HEIGHT && x>=0 && x<SCREEN_WIDTH) gfx_framebuffer[(y+i)*SCREEN_WIDTH + x] = 0xFFFFFF;
    
    // Bottom right shadow
    for (int i=0; i<w; i++) if (y+h-1>=0 && y+h-1<SCREEN_HEIGHT && x+i>=0 && x+i<SCREEN_WIDTH) gfx_framebuffer[(y+h-1)*SCREEN_WIDTH + (x+i)] = 0x222222;
    for (int i=0; i<h; i++) if (y+i>=0 && y+i<SCREEN_HEIGHT && x+w-1>=0 && x+w-1<SCREEN_WIDTH) gfx_framebuffer[(y+i)*SCREEN_WIDTH + (x+w-1)] = 0x222222;
}

void draw_string(int x, int y, const char *s, unsigned int color) {
    int ox = x;
    while (*s) {
        if (*s == '\n') { y += 10; x = ox; }
        else { draw_char(x, y, *s, color); x += 8; }
        s++;
    }
}

void draw_char_scaled(int x, int y, char c, unsigned int color, int scale) {
    if (c < 32 || c > 127) return;
    unsigned char *glyph = font[c - 32];
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (glyph[row] & (0x80 >> col))
                draw_rect(x + col*scale, y + row*scale, scale, scale, color);
        }
    }
}

void draw_string_scaled(int x, int y, const char *s, unsigned int color, int scale) {
    int ox = x;
    while (*s) {
        if (*s == '\n') { y += 10*scale; x = ox; }
        else { draw_char_scaled(x, y, *s, color, scale); x += 8*scale; }
        s++;
    }
}

static unsigned char cur_shape[16]={0b10000000,0b11000000,0b11100000,0b11110000,0b11111000,0b11111100,0b11111110,0b11111111,0b11111111,0b11111110,0b11001100,0b10000110,0b00000110,0b00000011,0b00000011,0b00000000};
static unsigned char cur_outline[16]={0b11000000,0b11100000,0b11110000,0b11111000,0b11111100,0b11111110,0b11111111,0b11111111,0b11111111,0b11111111,0b11101110,0b11001111,0b10001111,0b00000111,0b00000111,0b00000011};

static unsigned char hand_shape[16]={0b00110000,0b00110000,0b00110000,0b00110000,0b00110000,0b00111110,0b00111111,0b01111111,0b01111111,0b01111111,0b01111111,0b00111110,0b00111110,0b00011100,0b00011100,0b00000000};
static unsigned char hand_outline[16]={0b01111000,0b01111000,0b01111000,0b01111000,0b01111111,0b01111111,0b01111111,0b11111111,0b11111111,0b11111111,0b11111111,0b01111111,0b01111111,0b00111110,0b00111110,0b00000000};

static unsigned char ibeam_shape[16]={0b01111000,0b00110000,0b00110000,0b00110000,0b00110000,0b00110000,0b00110000,0b00110000,0b00110000,0b00110000,0b00110000,0b00110000,0b00110000,0b00110000,0b01111000,0b00000000};
static unsigned char ibeam_outline[16]={0b11111100,0b01111000,0b01111000,0b01111000,0b01111000,0b01111000,0b01111000,0b01111000,0b01111000,0b01111000,0b01111000,0b01111000,0b01111000,0b01111000,0b11111100,0b00000000};

int current_cursor_type = 0; // 0=arrow, 1=hand, 2=ibeam

void save_mouse_bg(int mx, int my) {
    mouse_saved_x = mx; mouse_saved_y = my;
    if(!gfx_framebuffer) return;
    for(int r=0; r<16; r++) {
        for(int c=0; c<16; c++) {
            if (mx+c < SCREEN_WIDTH && my+r < SCREEN_HEIGHT) {
                mouse_bg[r*16+c] = gfx_framebuffer[(my+r)*SCREEN_WIDTH + (mx+c)];
            }
        }
    }
}

void restore_mouse_bg(int mx, int my) {
    if(!gfx_framebuffer) return;
    for(int r=0; r<16; r++) {
        for(int c=0; c<16; c++) {
            if (mx+c < SCREEN_WIDTH && my+r < SCREEN_HEIGHT) {
                gfx_framebuffer[(my+r)*SCREEN_WIDTH + (mx+c)] = mouse_bg[r*16+c];
            }
        }
    }
}

void draw_mouse(int mx, int my) {
    if(!gfx_framebuffer) return;
    
    unsigned char *s = cur_shape;
    unsigned char *o = cur_outline;
    
    if (current_cursor_type == 1) { s = hand_shape; o = hand_outline; }
    else if (current_cursor_type == 2) { s = ibeam_shape; o = ibeam_outline; }
    
    for(int r=0; r<16; r++) {
        for(int c=0; c<8; c++) {
            if (mx+c < SCREEN_WIDTH && my+r < SCREEN_HEIGHT) {
                if (s[r] & (0x80 >> c)) {
                    gfx_framebuffer[(my+r)*SCREEN_WIDTH + (mx+c)] = 0xFFFFFF;
                } else if (o[r] & (0x80 >> c)) {
                    gfx_framebuffer[(my+r)*SCREEN_WIDTH + (mx+c)] = 0x000000;
                }
            }
        }
    }
}

void blur_region_alpha(int x, int y, int w, int h, int radius, unsigned int tint_color, unsigned char tint_alpha) {
    if(!gfx_framebuffer) return;
    
    // Optimized 2x2 downsampled blur for real-time performance on bare-metal CPU
    for(int r = 0; r < h; r += 2) {
        for(int c = 0; c < w; c += 2) {
            if (x + c < 0 || x + c >= SCREEN_WIDTH || y + r < 0 || y + r >= SCREEN_HEIGHT) continue;
            
            unsigned int r_tot = 0, g_tot = 0, b_tot = 0;
            int samples = 0;
            
            // Sample neighbors
            for(int dy = -radius; dy <= radius; dy += 2) {
                for(int dx = -radius; dx <= radius; dx += 2) {
                    int nx = x + c + dx;
                    int ny = y + r + dy;
                    if (nx >= 0 && nx < SCREEN_WIDTH && ny >= 0 && ny < SCREEN_HEIGHT) {
                        unsigned int px = gfx_backbuffer[ny * SCREEN_WIDTH + nx];
                        r_tot += (px >> 16) & 0xFF;
                        g_tot += (px >> 8) & 0xFF;
                        b_tot += px & 0xFF;
                        samples++;
                    }
                }
            }
            
            if (samples > 0) {
                unsigned int br = r_tot / samples;
                unsigned int bg = g_tot / samples;
                unsigned int bb = b_tot / samples;
                
                // Mix with tint
                unsigned int tr = (tint_color >> 16) & 0xFF;
                unsigned int tg = (tint_color >> 8) & 0xFF;
                unsigned int tb = tint_color & 0xFF;
                
                unsigned int out_r = (tr * tint_alpha + br * (255 - tint_alpha)) / 255;
                unsigned int out_g = (tg * tint_alpha + bg * (255 - tint_alpha)) / 255;
                unsigned int out_b = (tb * tint_alpha + bb * (255 - tint_alpha)) / 255;
                unsigned int final_col = (out_r << 16) | (out_g << 8) | out_b;
                
                // Write 2x2 block
                gfx_backbuffer[(y+r)*SCREEN_WIDTH + (x+c)] = final_col;
                if(x+c+1 < SCREEN_WIDTH) gfx_backbuffer[(y+r)*SCREEN_WIDTH + (x+c+1)] = final_col;
                if(y+r+1 < SCREEN_HEIGHT) gfx_backbuffer[(y+r+1)*SCREEN_WIDTH + (x+c)] = final_col;
                if(x+c+1 < SCREEN_WIDTH && y+r+1 < SCREEN_HEIGHT) gfx_backbuffer[(y+r+1)*SCREEN_WIDTH + (x+c+1)] = final_col;
            }
        }
    }
}

