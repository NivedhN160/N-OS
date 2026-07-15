#include "vga.h"
#include "fs.h"
#include "shell.h"
#include "auth.h"
#include "wm.h"
#include "rtc.h"
#include "process.h"
#include "multiboot.h"
#include "gdt.h"
#include "idt.h"
#include "isr.h"
#include "paging.h"
#include "pit.h"
#include "block.h"
#include "ata.h"
#include "vfs.h"
#include "fat32.h"
#include "compositor.h"

static inline void outb(unsigned short p,unsigned char v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static inline unsigned char inb(unsigned short p){unsigned char v;__asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));return v;}

// ── Mouse
static int k_scmp(const char *a, const char *b){
    while(*a&&*b&&*a==*b){a++;b++;}return *a-*b;
}

void mouse_wait(unsigned char t){unsigned int to=100000;if(t==0){while(to--)if(inb(0x64)&1)return;}else{while(to--)if(!(inb(0x64)&2))return;}}
void mouse_write(unsigned char d){mouse_wait(1);outb(0x64,0xD4);mouse_wait(1);outb(0x60,d);}
unsigned char mouse_read(){mouse_wait(0);return inb(0x60);}
#define KBD_BUF_SIZE 256
unsigned char kbd_buf[KBD_BUF_SIZE];
volatile int kbd_head = 0, kbd_tail = 0;

void keyboard_irq(registers_t *regs) {
    (void)regs;
    unsigned char status = inb(0x64);
    if (status & 1) {
        unsigned char sc = inb(0x60);
        kbd_buf[kbd_head] = sc;
        kbd_head = (kbd_head + 1) % KBD_BUF_SIZE;
    }
}

#define MOUSE_BUF_SIZE 256
unsigned char mouse_buf[MOUSE_BUF_SIZE];
volatile int mouse_head = 0, mouse_tail = 0;

void mouse_irq(registers_t *regs) {
    (void)regs;
    unsigned char status = inb(0x64);
    if (status & 0x20) {
        unsigned char sc = inb(0x60);
        mouse_buf[mouse_head] = sc;
        mouse_head = (mouse_head + 1) % MOUSE_BUF_SIZE;
    }
}

void mouse_init(){
    mouse_wait(1);outb(0x64,0xA8);
    mouse_wait(1);outb(0x64,0x20);mouse_wait(0);
    unsigned char s=inb(0x60)|3;
    mouse_wait(1);outb(0x64,0x60);mouse_wait(1);outb(0x60,s);
    mouse_write(0xF6);mouse_read();
    mouse_write(0xF4);mouse_read();
}

// ── Keyboard
char sc2ascii(unsigned char sc){
    static char map[58]={0,0,'1','2','3','4','5','6','7','8','9','0','-','=','\b','\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' '};
    if(sc<58)return map[sc];return 0;
}

// ── Terminal
#define TERM_MAX_COLS 48
#define TERM_MAX_ROWS 25
char term_buf[TERM_MAX_ROWS][TERM_MAX_COLS];
unsigned int term_col[TERM_MAX_ROWS][TERM_MAX_COLS];
int tx=0,ty=0;

void term_newline(){
    ty++; tx=0;
    if(ty>=TERM_MAX_ROWS){
        for(int r=1;r<TERM_MAX_ROWS;r++)
            for(int c=0;c<TERM_MAX_COLS;c++){
                term_buf[r-1][c]=term_buf[r][c];
                term_col[r-1][c]=term_col[r][c];
            }
        ty=TERM_MAX_ROWS-1;
        for(int c=0;c<TERM_MAX_COLS;c++) term_buf[ty][c]=0;
    }
}
void term_putchar(char c,unsigned int col){
    if(c=='\n'){term_newline();return;}
    term_buf[ty][tx]=c; term_col[ty][tx]=col;
    tx++; if(tx>=TERM_MAX_COLS) term_newline();
}
void term_print(const char *s,unsigned int col){
    while(*s)term_putchar(*s++,col);
}
void term_clear(){
    for(int r=0;r<TERM_MAX_ROWS;r++)
        for(int c=0;c<TERM_MAX_COLS;c++) term_buf[r][c]=0;
    tx=0;ty=0;
}

void term_on_draw(int wx, int wy, int ww, int wh) {
    draw_rect(wx, wy, ww, wh, 0x000000);
    for(int r=0;r<TERM_MAX_ROWS;r++)
        for(int c=0;c<TERM_MAX_COLS;c++){
            char ch = term_buf[r][c];
            if(ch) draw_char(wx+c*8+2, wy+r*10+2, ch, term_col[r][c]);
        }
}

char cmd_buf[64];
int cmd_len = 0;
void term_on_key(char c) {
    if(c=='\n'){
        term_print("\n",0xFFFFFF);
        cmd_buf[cmd_len]=0;
        shell_handle(cmd_buf);
        cmd_len=0;
        term_print("/",0x00FF00); term_print(fs[current_dir].name,0x00FF00); term_print(" > ",0x00FF00);
    } else if(c=='\b'&&cmd_len>0){
        cmd_len--; tx--;
        if(tx<0){tx=0;}
        term_buf[ty][tx]=0;
    } else if(c&&c!='\t'&&cmd_len<63){
        char s[2]={c,0};
        term_print(s,0xFFFFFF);
        cmd_buf[cmd_len++]=c;
    }
}

// ── File Manager
int fm_ctx_menu_open = 0;
int fm_ctx_x = 0, fm_ctx_y = 0;
int fm_ctx_file_idx = -1;
char fm_search_buf[32] = {0};
int fm_search_len = 0;

void file_on_key(char c) {
    if (c == '\b' && fm_search_len > 0) {
        fm_search_buf[--fm_search_len] = 0;
    } else if (c && c != '\t' && c != '\n' && fm_search_len < 30) {
        fm_search_buf[fm_search_len++] = c;
        fm_search_buf[fm_search_len] = 0;
    }
}

void file_on_draw(int wx, int wy, int ww, int wh) {
    draw_rect(wx, wy, ww, wh, 0xFFFFFF);
    draw_string(wx+5, wy+5, "Drive: C: (/root)", 0x0000FF);
    
    // Search bar
    draw_string(wx+150, wy+5, "Search: ", 0x000000);
    draw_rect_alpha(wx+210, wy+3, 150, 16, 0xEEEEEE, 255);
    draw_string(wx+215, wy+7, fm_search_buf, 0x000000);
    
    draw_rect(wx+5, wy+20, ww-10, 1, 0xAAAAAA);
    
    // Draw "Create File" button graphically
    draw_rounded_rect_alpha(wx+ww-100, wy+3, 90, 16, 4, 0xEEEEEE, 255);
    draw_string(wx+ww-95, wy+7, "+ New File", 0x000000);
    
    int y = wy + 25;
    for (int i=0; i<MAX_FILES; i++) {
        if (fs[current_dir].files[i].used) {
            // Filter
            if (fm_search_len > 0) {
                int match = 0;
                for(int j=0; fs[current_dir].files[i].name[j]; j++) {
                    int k=0;
                    while(fm_search_buf[k] && fs[current_dir].files[i].name[j+k] == fm_search_buf[k]) k++;
                    if (!fm_search_buf[k]) { match = 1; break; }
                }
                if (!match) continue;
            }
            draw_rect(wx+5, y, 10, 12, 0xDDDDDD); // document icon
            draw_rect(wx+7, y+2, 6, 1, 0x555555);
            draw_rect(wx+7, y+5, 6, 1, 0x555555);
            draw_rect(wx+7, y+8, 6, 1, 0x555555);
            
            draw_string(wx+20, y, fs[current_dir].files[i].name, 0x000000);
            y += 20;
        }
    }
    
    if (fm_ctx_menu_open) {
        draw_shadow_rect(wx + fm_ctx_x, wy + fm_ctx_y, 100, 60);
        draw_rounded_rect_alpha(wx + fm_ctx_x, wy + fm_ctx_y, 100, 60, 4, 0xFFFFFF, 230); // Glass menu
        draw_string(wx + fm_ctx_x + 10, wy + fm_ctx_y + 5, "Open", 0x000000);
        draw_string(wx + fm_ctx_x + 10, wy + fm_ctx_y + 20, "Execute", 0x000000);
        draw_string(wx + fm_ctx_x + 10, wy + fm_ctx_y + 35, "Properties", 0x000000);
    }
}

void file_on_mouse(int rx, int ry, int mb) {
    if (fm_ctx_menu_open) {
        if (mb & 1) { // Left click on context menu
            if (rx >= fm_ctx_x && rx <= fm_ctx_x + 100 && ry >= fm_ctx_y && ry <= fm_ctx_y + 60) {
                if (ry < fm_ctx_y + 15) {
                    term_print("Opened file: ", 0xFFFFFF); term_print(fs[current_dir].files[fm_ctx_file_idx].name, 0x00FFFF); term_print("\n", 0xFFFFFF);
                } else if (ry < fm_ctx_y + 30) {
                    term_print("Executing script: ", 0xFFFFFF); term_print(fs[current_dir].files[fm_ctx_file_idx].name, 0x00FF00); term_print("\n", 0xFFFFFF);
                }
            }
            fm_ctx_menu_open = 0; // Close menu
        }
        return;
    }

    if (mb & 1) { // Left click
        if (rx > 300 && ry > 3 && ry < 19) {
            fs_touch("new_gui_file.txt", "Created by GUI\n");
            return;
        }
    }
    
    if (mb & 2) { // Right click
        int y = 25;
        for (int i=0; i<MAX_FILES; i++) {
            if (fs[current_dir].files[i].used) {
                if (ry >= y && ry < y+20) {
                    fm_ctx_menu_open = 1;
                    fm_ctx_x = rx;
                    fm_ctx_y = ry;
                    fm_ctx_file_idx = i;
                    return;
                }
                y += 20;
            }
        }
    }
}

// ── This PC
void this_pc_on_draw(int wx, int wy, int ww, int wh) {
    draw_rect(wx, wy, ww, wh, 0xFFFFFF);
    draw_string(wx+20, wy+20, "This Computer", 0x000000);
    draw_rect(wx+20, wy+35, ww-40, 1, 0xAAAAAA);
    draw_string(wx+20, wy+45, "OS: N-OS v1.2", 0x555555);
    draw_string(wx+20, wy+60, "CPU: 64-bit x86_64 (Simulated)", 0x555555);
    draw_string(wx+20, wy+75, "RAM: 32 MB", 0x555555);
    draw_string(wx+20, wy+90, "VGA: 1024x768 32-bpp", 0x555555);
}

// ── Task Manager
void taskmgr_on_draw(int wx, int wy, int ww, int wh) {
    draw_rect(wx, wy, ww, wh, 0xFFFFFF);
    draw_string(wx+5, wy+5, "PID  NAME", 0x0000FF);
    draw_rect(wx+5, wy+15, ww-10, 1, 0xAAAAAA);
    
    int y = wy + 20;
    for(int i=0; i<MAX_PROCESSES; i++) {
        if (processes[i].active) {
            char pid_str[3];
            pid_str[0] = (processes[i].id / 10) + '0';
            pid_str[1] = (processes[i].id % 10) + '0';
            pid_str[2] = 0;
            draw_string(wx+5, y, pid_str, 0x000000);
            draw_string(wx+40, y, processes[i].name, 0x000000);
            y += 10;
        }
    }
}

// ── Package Manager State
int app_installed_firefox = 0;
int app_installed_calc = 1;

// ── Calculator
char calc_display[32] = "0";
int calc_acc = 0;
int calc_op = 0; // 0=none, 1=+, 2=-
int calc_new_num = 1;

void calc_on_draw(int wx, int wy, int ww, int wh) {
    draw_rect_alpha(wx, wy, ww, wh, 0xFFFFFF, 220); // glass
    draw_rect(wx+5, wy+5, ww-10, 20, 0xFFFFFF); // screen
    draw_rect(wx+5, wy+5, ww-10, 1, 0xAAAAAA);
    
    // measure len
    int l=0; while(calc_display[l]) l++;
    draw_string(wx+ww-15 - l*8, wy+10, calc_display, 0x000000);
    
    for(int i=0; i<3; i++) {
        for(int j=0; j<3; j++) {
            draw_rounded_rect_alpha(wx+10+j*40, wy+40+i*30, 30, 20, 5, 0xDDDDDD, 255);
            char b[2] = {'1'+i*3+j, 0};
            draw_string(wx+20+j*40, wy+45+i*30, b, 0x000000);
        }
    }
    // + - =
    draw_rounded_rect_alpha(wx+130, wy+40, 30, 20, 5, 0xAAAAAA, 255); draw_string(wx+140, wy+45, "+", 0);
    draw_rounded_rect_alpha(wx+130, wy+70, 30, 20, 5, 0xAAAAAA, 255); draw_string(wx+140, wy+75, "-", 0);
    draw_rounded_rect_alpha(wx+130, wy+100, 30, 20, 5, 0xFFAA00, 255); draw_string(wx+140, wy+105, "=", 0);
}
void itoa(int val, char* buf) {
    if (val == 0) { buf[0]='0'; buf[1]=0; return; }
    int i=0; 
    if(val<0){ buf[i++]='-'; val=-val; }
    int t=val, len=0;
    while(t){ len++; t/=10; }
    for(int j=0; j<len; j++) {
        buf[i+len-1-j] = (val%10)+'0';
        val/=10;
    }
    buf[i+len]=0;
}
void calc_on_mouse(int rx, int ry, int mb) {
    if (!mb) return;
    int cur_val = 0;
    int i=0; while(calc_display[i]){ cur_val=cur_val*10+(calc_display[i]-'0'); i++; }
    
    for(int i=0; i<3; i++) {
        for(int j=0; j<3; j++) {
            if (rx >= 10+j*40 && rx <= 40+j*40 && ry >= 40+i*30 && ry <= 60+i*30) {
                if(calc_new_num){ calc_display[0]='1'+i*3+j; calc_display[1]=0; calc_new_num=0; }
                else {
                    int l=0; while(calc_display[l]) l++;
                    if(l<10){ calc_display[l]='1'+i*3+j; calc_display[l+1]=0; }
                }
                return;
            }
        }
    }
    if (rx >= 130 && rx <= 160 && ry >= 40 && ry <= 60) { // +
        calc_acc = cur_val; calc_op = 1; calc_new_num = 1;
    }
    if (rx >= 130 && rx <= 160 && ry >= 70 && ry <= 90) { // -
        calc_acc = cur_val; calc_op = 2; calc_new_num = 1;
    }
    if (rx >= 130 && rx <= 160 && ry >= 100 && ry <= 120) { // =
        if (calc_op == 1) itoa(calc_acc + cur_val, calc_display);
        if (calc_op == 2) itoa(calc_acc - cur_val, calc_display);
        calc_op = 0; calc_new_num = 1;
    }
}

// ── Notepad
char notepad_buf[256];
int notepad_len = 0;
void notepad_on_draw(int wx, int wy, int ww, int wh) {
    draw_rect_alpha(wx, wy, ww, wh, 0xFFFFFF, 230); // frosted white
    
    // Save button
    draw_rounded_rect_alpha(wx+ww-60, wy+5, 50, 20, 5, 0xDDDDDD, 255);
    draw_string(wx+ww-50, wy+10, "Save", 0x000000);
    
    draw_string(wx+5, wy+30, notepad_buf, 0x000000);
}
void notepad_on_mouse(int rx, int ry, int mb) {
    if (mb == 3) { // Left + Right click (Plan 9 Chording)
        extern char clipboard[5][32];
        extern int clipboard_head;
        extern int slen(const char*);
        int l = slen(clipboard[clipboard_head]);
        for(int i=0; i<l && notepad_len < 255; i++) {
            notepad_buf[notepad_len++] = clipboard[clipboard_head][i];
        }
        notepad_buf[notepad_len] = 0;
        return;
    }
    if (mb && rx >= 340 && rx <= 390 && ry >= 5 && ry <= 25) {
        // Save
        extern void fs_write_file(const char*, const char*);
        fs_write_file("/root/note.txt", notepad_buf);
    }
}
void notepad_on_key(char c) {
    if (c == '\b' && notepad_len > 0) notepad_buf[--notepad_len] = 0;
    else if (c && notepad_len < 255) { notepad_buf[notepad_len++] = c; notepad_buf[notepad_len]=0; }
}

// ── Settings
void settings_on_draw(int wx, int wy, int ww, int wh) {
    draw_rect_alpha(wx, wy, ww, wh, 0xFFFFFF, 230);
    draw_string(wx+10, wy+10, "Personalization", 0x000000);
    
    extern int wallpaper_type;
    
    draw_rounded_rect_alpha(wx+20, wy+40, 150, 30, 5, wallpaper_type==0 ? 0xAAAAAA : 0xDDDDDD, 255);
    draw_string(wx+30, wy+50, "1. Glass Gradient", 0x000000);
    
    draw_rounded_rect_alpha(wx+20, wy+80, 150, 30, 5, wallpaper_type==1 ? 0xAAAAAA : 0xDDDDDD, 255);
    draw_string(wx+30, wy+90, "2. Solid Dark", 0x000000);
    
    draw_rounded_rect_alpha(wx+20, wy+120, 150, 30, 5, wallpaper_type==2 ? 0xAAAAAA : 0xDDDDDD, 255);
    draw_string(wx+30, wy+130, "3. Solid Blue", 0x000000);
    
    draw_rounded_rect_alpha(wx+20, wy+160, 150, 30, 5, wallpaper_type==3 ? 0xAAAAAA : 0xDDDDDD, 255);
    draw_string(wx+30, wy+170, "4. TempleOS Mode", 0x0000FF);
}
void settings_on_mouse(int rx, int ry, int mb) {
    if (!mb) return;
    extern int wallpaper_type;
    if (rx >= 20 && rx <= 170) {
        if (ry >= 40 && ry <= 70) wallpaper_type = 0;
        if (ry >= 80 && ry <= 110) wallpaper_type = 1;
        if (ry >= 120 && ry <= 150) wallpaper_type = 2;
        if (ry >= 160 && ry <= 190) wallpaper_type = 3;
    }
}

// ── Firefox Browser (Offline GUI Mockup)
char firefox_url[128] = "https://www.google.com";

int firefox_url_len = 22;
int firefox_state = 0; // 0 = Google, 1 = Loading, 2 = Loaded

void firefox_on_key(char c) {
    if (c == '\b' && firefox_url_len > 0) {
        firefox_url[--firefox_url_len] = 0;
    } else if (c == '\n') {
        firefox_state = 1; // Loading
        void net_send_http_request(const char *);
        net_send_http_request(firefox_url);
    } else if (c >= 32 && c <= 126 && firefox_url_len < 127) {
        firefox_url[firefox_url_len++] = c;
        firefox_url[firefox_url_len] = 0;
    }
}

void firefox_on_draw(int wx, int wy, int ww, int wh) {
    draw_rect(wx, wy, ww, wh, 0xFFFFFF);
    
    // Top bar (Tab area)
    draw_rect(wx, wy, ww, 30, 0x202340); // Dark Firefox theme
    draw_rounded_rect_alpha(wx+10, wy+5, 150, 25, 5, 0x40445A, 255);
    draw_string(wx+15, wy+12, "New Tab", 0xFFFFFF);
    
    // Navigation bar
    draw_rect(wx, wy+30, ww, 40, 0x40445A);
    draw_rounded_rect_alpha(wx+60, wy+38, ww-80, 24, 12, 0x1C1B22, 255);
    
    // Back / Forward buttons
    draw_string(wx+10, wy+42, "<-", 0xAAAAAA);
    draw_string(wx+35, wy+42, "->", 0xAAAAAA);
    
    // URL
    draw_string(wx+70, wy+44, firefox_url, 0xFFFFFF);
    
    // Page Content
    if (firefox_state == 0) {
        draw_string_scaled(wx + ww/2 - 80, wy + 150, "Google", 0x000000, 4);
        draw_rect(wx + ww/2 - 120, wy + 200, 240, 25, 0xFFFFFF);
        draw_rect(wx + ww/2 - 120, wy + 200, 240, 1, 0xCCCCCC);
        draw_rect(wx + ww/2 - 120, wy + 225, 240, 1, 0xCCCCCC);
        draw_rect(wx + ww/2 - 120, wy + 200, 1, 25, 0xCCCCCC);
        draw_rect(wx + ww/2 + 120, wy + 200, 1, 25, 0xCCCCCC);
        draw_string(wx + ww/2 - 80, wy + 240, "Search the web (Offline)", 0x888888);
    } else if (firefox_state == 1) {
        draw_string_scaled(wx + ww/2 - 60, wy + 200, "Loading...", 0x888888, 2);
    } else if (firefox_state == 2) {
        draw_string_scaled(wx + 20, wy + 100, "TCP Connection Established!", 0x00AA00, 1);
        draw_string(wx + 20, wy + 120, "Downloading application package...", 0x0000FF);
        
        // Simulate download
        fs_touch("N-OS_App.exe", "MZ... PE... N-OS Executable!");
        
        draw_rounded_rect_alpha(wx + 20, wy + 150, 200, 40, 5, 0xDDFFDD, 255);
        draw_string(wx + 30, wy + 160, "Download Complete!", 0x005500);
        draw_string(wx + 30, wy + 175, "Saved as N-OS_App.exe", 0x005500);
    }
}

// ── App Store
void app_store_on_draw(int wx, int wy, int ww, int wh) {
    draw_rect(wx, wy, ww, wh, 0xFFFFFF);
    draw_rect(wx, wy, ww, 40, 0x0078D7); // Blue header
    draw_string(wx+20, wy+15, "N-OS Software Center", 0xFFFFFF);
    
    // Firefox listing
    draw_rect(wx+20, wy+60, ww-40, 60, 0xEEEEEE);
    draw_string(wx+30, wy+70, "Firefox Browser", 0x000000);
    draw_string(wx+30, wy+85, "A fast, open-source web browser.", 0x555555);
    
    // Install/Remove button
    draw_rounded_rect_alpha(wx+ww-120, wy+75, 80, 25, 5, app_installed_firefox ? 0xAA0000 : 0x00AA00, 255);
    draw_string(wx+ww-110, wy+80, app_installed_firefox ? "Remove" : "Install", 0xFFFFFF);
}

void app_store_on_mouse(int rx, int ry, int mb) {
    if (mb == 1) {
        if (rx >= 280 && rx <= 360 && ry >= 75 && ry <= 100) {
            app_installed_firefox = !app_installed_firefox;
        }
    }
}

// ── Boot sequence
void run_boot_sequence() {
    int scale = 12;
    int letter_w = 8 * scale;
    int target_nx = (SCREEN_WIDTH / 2) - (letter_w * 2);
    int target_my = SCREEN_HEIGHT / 2 - (4 * scale);
    int target_ox = (SCREEN_WIDTH / 2) + letter_w;

    for (int frame = 0; frame <= 100; frame += 2) {
        clear_screen(0x000000);
        
        // N comes from left
        int nx = (frame * target_nx) / 100;
        int ny = target_my;
        
        // - comes from top
        int mx = target_nx + letter_w;
        int my = (frame * target_my) / 100;
        
        // OS comes from right
        int ox = SCREEN_WIDTH - (frame * (SCREEN_WIDTH - target_ox)) / 100;
        int oy = target_my;
        
        draw_string_scaled(nx, ny, "N", 0xFFFFFF, scale);
        draw_string_scaled(mx, my, "-", 0x00AA00, scale);
        draw_string_scaled(ox, oy, "OS", 0xFFFFFF, scale);
        
        gfx_swap();
        for(volatile int d=0; d<20000000; d++); // majestic slow delay
    }
    
    // Light beam sweep across the logo
    for (int beam = target_nx - 100; beam < target_ox + 200; beam += 5) {
        clear_screen(0x000000);
        draw_string_scaled(target_nx, target_my, "N", 0xFFFFFF, scale);
        draw_string_scaled(target_nx + letter_w, target_my, "-", 0x00AA00, scale);
        draw_string_scaled(target_ox, target_my, "OS", 0xFFFFFF, scale);
        
        // Draw angled translucent beam
        for (int i=0; i<60; i++) {
            draw_rect_alpha(beam + i, target_my - 20, 1, scale*12, 0xFFFFFF, (i < 30) ? (i*4) : ((60-i)*4));
        }
        gfx_swap();
        for(volatile int d=0; d<4000000; d++); // slower beam
    }
    
    for(volatile int d=0; d<50000000; d++);
}

void run_shutdown() {
    int scale = 12;
    int letter_w = 8 * scale;
    int target_nx = (SCREEN_WIDTH / 2) - (letter_w * 2);
    int target_my = SCREEN_HEIGHT / 2 - (4 * scale);
    int target_ox = (SCREEN_WIDTH / 2) + letter_w;

    // Light beam sweep out
    for (int beam = target_ox + 200; beam > target_nx - 100; beam -= 5) {
        clear_screen(0x000000);
        draw_string_scaled(target_nx, target_my, "N", 0xFFFFFF, scale);
        draw_string_scaled(target_nx + letter_w, target_my, "-", 0x00AA00, scale);
        draw_string_scaled(target_ox, target_my, "OS", 0xFFFFFF, scale);
        
        for (int i=0; i<60; i++) {
            draw_rect_alpha(beam + i, target_my - 20, 1, scale*12, 0xFFFFFF, (i < 30) ? (i*4) : ((60-i)*4));
        }
        gfx_swap();
        for(volatile int d=0; d<4000000; d++); 
    }
    
    // Reverse fly-out
    for (int frame = 100; frame >= 0; frame -= 2) {
        clear_screen(0x000000);
        
        int nx = (frame * target_nx) / 100;
        int ny = target_my;
        int mx = target_nx + letter_w;
        int my = (frame * target_my) / 100;
        int ox = SCREEN_WIDTH - (frame * (SCREEN_WIDTH - target_ox)) / 100;
        int oy = target_my;
        
        if (frame > 10) draw_string_scaled(nx, ny, "N", 0xFFFFFF, scale);
        if (frame > 10) draw_string_scaled(mx, my, "-", 0x00AA00, scale);
        if (frame > 10) draw_string_scaled(ox, oy, "OS", 0xFFFFFF, scale);
        
        gfx_swap();
        for(volatile int d=0; d<20000000; d++); 
    }
    
    clear_screen(0x000000);
    gfx_swap();

    // Trigger ACPI Shutdown
    __asm__ volatile("outw %0, %1" : : "a"((unsigned short)0x2000), "Nd"((unsigned short)0xB004)); // Bochs/QEMU/older VBox
    __asm__ volatile("outw %0, %1" : : "a"((unsigned short)0x3400), "Nd"((unsigned short)0x4004)); // PIIX4 VBox
    __asm__ volatile("outw %0, %1" : : "a"((unsigned short)0x2000), "Nd"((unsigned short)0x604));  // PIIX4 alternative
    
    while(1) { __asm__ volatile("hlt"); }
}

// Async Login Globals
char login_uname[32] = {0};
char login_upass[32] = {0};
char login_cpass[32] = {0};
int login_u_len = 0;
int login_p_len = 0;
int login_c_len = 0;
int login_focus = 0; // 0=username, 1=password, 2=confirm
int login_error = 0;

void draw_login(int is_register) {
    clear_screen(0x111133); // deep blue background

    int lx = SCREEN_WIDTH/2 - 150;
    int ly = SCREEN_HEIGHT/2 - (is_register ? 120 : 100);
    int pane_h = is_register ? 270 : 220;
    
    draw_shadow_rect(lx, ly, 300, pane_h);
    draw_rounded_rect_alpha(lx, ly, 300, pane_h, 10, 0xFFFFFF, 230); // frosted glass pane
    
    draw_string(lx + 80, ly + 20, is_register ? "First Boot: Register" : "Login to N-OS", 0x000000);

    draw_string(lx + 40, ly + 60, "Username:", 0x333333);
    draw_rect(lx + 40, ly + 75, 220, 20, (login_focus==0) ? 0xEEEEFF : 0xFFFFFF);
    draw_rect(lx + 40, ly + 75, 220, 1, 0xCCCCCC);
    draw_string(lx + 45, ly + 81, login_uname, 0x000000);

    draw_string(lx + 40, ly + 110, "Password:", 0x333333);
    draw_rect(lx + 40, ly + 125, 220, 20, (login_focus==1) ? 0xEEEEFF : 0xFFFFFF);
    draw_rect(lx + 40, ly + 125, 220, 1, 0xCCCCCC);
    
    char stars[32];
    for(int i=0; i<login_p_len; i++) stars[i] = '*';
    stars[login_p_len] = 0;
    draw_string(lx + 45, ly + 131, stars, 0x000000);

    int btn_y = ly + 160;

    if (is_register) {
        draw_string(lx + 40, ly + 160, "Confirm Password:", 0x333333);
        draw_rect(lx + 40, ly + 175, 220, 20, (login_focus==2) ? 0xEEEEFF : 0xFFFFFF);
        draw_rect(lx + 40, ly + 175, 220, 1, 0xCCCCCC);
        
        char cstars[32];
        for(int i=0; i<login_c_len; i++) cstars[i] = '*';
        cstars[login_c_len] = 0;
        draw_string(lx + 45, ly + 181, cstars, 0x000000);
        btn_y = ly + 210;
    }

    draw_rounded_rect_alpha(lx + 110, btn_y, 80, 24, 6, 0x0078D7, 255); // windows blue button
    draw_string(lx + 125, btn_y + 8, is_register ? "REGISTER" : "LOGIN", 0xFFFFFF);
    
    // Graphical Power Options
    draw_rounded_rect_alpha(lx + 40, btn_y + 35, 14, 14, 7, 0xFF0000, 255);
    draw_string(lx + 60, btn_y + 38, "Reboot", 0x444444);
    
    draw_rounded_rect_alpha(lx + 200, btn_y + 35, 14, 14, 7, 0xFF0000, 255);
    draw_string(lx + 220, btn_y + 38, "Shutdown", 0x444444);
    
    if (login_error == 1) {
        draw_string(lx + 80, pane_h + ly - 20, "Wrong credentials!", 0xFF0000);
    } else if (login_error == 2) {
        draw_string(lx + 80, pane_h + ly - 20, "Passwords do not match!", 0xFF0000);
    }
}

// ── Dummy Background processes
void idle_task() {}
void window_manager_task() {}

char clipboard_ring[5][256] = {
    "Welcome to N-OS",
    "https://google.com",
    "sudo rm -rf /",
    "Hello World!",
    ""
};
int clipboard_head = 0;

void win_v_on_draw(int wx, int wy, int ww, int wh) {
    draw_rect(wx, wy, ww, wh, 0xFFFFFF);
    draw_string(wx+5, wy+5, "Clipboard History", 0x333333);
    for(int i=0; i<5; i++) {
        draw_rect(wx+5, wy+25 + i*20, ww-10, 18, 0xEEEEEE);
        draw_string(wx+10, wy+30 + i*20, clipboard_ring[(clipboard_head + i) % 5], 0x000000);
    }
}

void scopy(char *d, const char *s) {
    int i=0; 
    while(s[i]){d[i]=s[i]; i++;}
    d[i]=0;
}

void kernel_main(unsigned int magic, multiboot_info_t* mbi){
    if (magic != 0x2BADB002) return;
    
    gdt_init();
    idt_init();
    paging_init(); // Phase 1: Virtual Memory!
    pit_init(100); // Phase 1: Preemptive Scheduler Hardware Timer (100Hz)
    
    // Storage & Filesystem Subsystem
    ata_init();
    vfs_init();
    // fat32_init(0); // Optional: if we had a real disk attached
    
    // UI Subsystem
    compositor_init();
    
    register_interrupt_handler(33, keyboard_irq);
    register_interrupt_handler(44, mouse_irq);
    __asm__ volatile("sti");
    
    // Init VBE High Def Graphics
    gfx_init(mbi->framebuffer_addr, mbi->framebuffer_pitch);
    // rtc_init();
    
    run_boot_sequence();
    
    auth_init();
    fs_init();
    process_init();
    void pci_init();
    pci_init();
    void security_init();
    security_init();

    int lx = SCREEN_WIDTH/2 - 150;
    int ly = SCREEN_HEIGHT/2 - 100;
    // ── Pre-install essential files
    fs_touch("readme.txt", "Welcome to N-OS Graphical Environment!\nEnjoy the fast, secure experience.\n");
    fs_touch("system.log", "Booted successfully.\nVFS initialized.\nGUI started.\n");
    fs_touch("secret.doc", "Top secret: The cake is a lie.\n");
    
    // Create a real Windows PE executable structure for app_win.exe
    // Minimum 64 bytes for DOS header, plus PE signature
    char fake_exe[128];
    for (int i=0; i<128; i++) fake_exe[i] = 0;
    fake_exe[0] = 0x4D; // M
    fake_exe[1] = 0x5A; // Z
    fake_exe[60] = 64;  // e_lfanew
    fake_exe[64] = 'P';
    fake_exe[65] = 'E';
    fs_touch("app_win.exe", fake_exe);
    
    wm_init();
    process_create("Idle Process", idle_task);
    process_create("Window Manager", window_manager_task);
    process_create("File System", idle_task);
    
    term_clear();
    term_print("           _nnnn_\n", 0x00FF00);
    term_print("          dGGGGMMb     ", 0x00FF00); term_print("OS: ", 0x00FFFF); term_print("N-OS Ultimate Edition\n", 0xFFFFFF);
    term_print("         @p~qp~~qMb    ", 0x00FF00); term_print("Host: ", 0x00FFFF); term_print("x86 Bare Metal (Ring 0)\n", 0xFFFFFF);
    term_print("         M|@||@) M|    ", 0x00FF00); term_print("Kernel: ", 0x00FFFF); term_print("N-OS Monolithic\n", 0xFFFFFF);
    term_print("         @,----.JM|    ", 0x00FF00); term_print("Uptime: ", 0x00FFFF); term_print("Just booted\n", 0xFFFFFF);
    term_print("        JS^\\__/  qKL   ", 0x00FF00); term_print("Shell: ", 0x00FFFF); term_print("nshell 1.2\n", 0xFFFFFF);
    term_print("       dZP        qKRb ", 0x00FF00); term_print("Resolution: ", 0x00FFFF); term_print("1024x768\n", 0xFFFFFF);
    term_print("      dZP          qKKb", 0x00FF00); term_print("WM: ", 0x00FFFF); term_print("N-WM (Glassmorphism)\n", 0xFFFFFF);
    term_print("\nWelcome ", 0xFFFF55);
    term_print(current_user, 0xFFFF55);
    term_print("! Type 'help' for commands.\n", 0xAAAAAA);
    term_print("/root > ", 0x00FF00);

    int tw = wm_create_window(100, 100, 600, 400, "Terminal", term_on_draw);
    wm_set_callbacks(tw, 0, term_on_key);

    mouse_init();
    unsigned char mc=0;
    signed char mb[3];
    int cur_x = SCREEN_WIDTH/2, cur_y = SCREEN_HEIGHT/2;
    int prev_x = cur_x, prev_y = cur_y;
    int dirty = 1;
    int ctrl_held = 0;
    int win_held = 0;
    
    while (1) {
        int processed = 0;
        while (mouse_head != mouse_tail) {
            unsigned char d = mouse_buf[mouse_tail];
            mouse_tail = (mouse_tail + 1) % MOUSE_BUF_SIZE;
            processed = 1;
            
            if (mc == 0 && !(d & 0x08)) continue; // Fix out of sync packets
            
            mb[mc++]=(signed char)d;
            if(mc==3){
                mc=0;
                cur_x+=(signed char)mb[1];
                cur_y-=(signed char)mb[2];
                if(cur_x<0)cur_x=0;if(cur_x>SCREEN_WIDTH-16)cur_x=SCREEN_WIDTH-16;
                if(cur_y<0)cur_y=0;if(cur_y>SCREEN_HEIGHT-16)cur_y=SCREEN_HEIGHT-16;
                
                int m_btn = mb[0] & 7;
                
                extern int current_cursor_type;
                current_cursor_type = 0; // Default arrow
                
                if (!logged_in) {
                    int lx = SCREEN_WIDTH/2 - 150;
                    int ly = SCREEN_HEIGHT/2 - (users_count == 0 ? 120 : 100);
                    int btn_y = ly + (users_count == 0 ? 210 : 160);
                    
                    if (cur_x >= lx + 40 && cur_x <= lx + 260 && cur_y >= ly + 75 && cur_y <= ly + 95) current_cursor_type = 2; // I-beam
                    else if (cur_x >= lx + 40 && cur_x <= lx + 260 && cur_y >= ly + 125 && cur_y <= ly + 145) current_cursor_type = 2; // I-beam
                    else if (users_count == 0 && cur_x >= lx + 40 && cur_x <= lx + 260 && cur_y >= ly + 175 && cur_y <= ly + 195) current_cursor_type = 2; // I-beam
                    else if (cur_x >= lx + 110 && cur_x <= lx + 190 && cur_y >= btn_y && cur_y <= btn_y + 24) current_cursor_type = 1; // Hand
                    else if (cur_y >= btn_y + 35 && cur_y <= btn_y + 55 && (cur_x >= lx + 40 && cur_x <= lx + 60 || cur_x >= lx + 200 && cur_x <= lx + 220)) current_cursor_type = 1; // Hand

                    if (m_btn & 1) {
                        if (cur_x >= lx + 40 && cur_x <= lx + 260 && cur_y >= ly + 75 && cur_y <= ly + 95) login_focus = 0;
                        else if (cur_x >= lx + 40 && cur_x <= lx + 260 && cur_y >= ly + 125 && cur_y <= ly + 145) login_focus = 1;
                        else if (users_count == 0 && cur_x >= lx + 40 && cur_x <= lx + 260 && cur_y >= ly + 175 && cur_y <= ly + 195) login_focus = 2;
                        else if (cur_x >= lx + 110 && cur_x <= lx + 190 && cur_y >= btn_y && cur_y <= btn_y + 24) {
                            if (users_count == 0) {
                                int match = 1;
                                for(int k=0; k<32; k++) { if (login_upass[k] != login_cpass[k]) match = 0; }
                                if (login_p_len == 0 || login_u_len == 0 || !match) {
                                    login_error = 2;
                                } else {
                                    auth_add_user(login_uname, login_upass);
                                    auth_login(login_uname, login_upass);
                                    login_error = 0;
                                }
                            } else {
                                if (!auth_login(login_uname, login_upass)) login_error = 1;
                                else { login_uname[0]=0; login_upass[0]=0; login_cpass[0]=0; login_u_len=0; login_p_len=0; login_c_len=0; }
                            }
                        } else if (cur_x >= lx + 40 && cur_x <= lx + 60 && cur_y >= btn_y + 35 && cur_y <= btn_y + 55) {
                            outb(0x64, 0xFE);
                        } else if (cur_x >= lx + 200 && cur_x <= lx + 220 && cur_y >= btn_y + 35 && cur_y <= btn_y + 55) {
                            run_shutdown();
                        }
                    }
                    if (m_btn) dirty = 1;
                } else {
                    int start_action = wm_handle_mouse(cur_x, cur_y, m_btn, mb[1], mb[2]);
                    
                    if (start_action) {
                        if (start_action == 1) {
                            int w = wm_create_window(120, 120, 600, 400, "Terminal", term_on_draw);
                            wm_set_callbacks(w, 0, term_on_key);
                        } else if (start_action == 2) {
                            int w = wm_create_window(140, 140, 500, 300, "File Manager", file_on_draw);
                            wm_set_callbacks(w, file_on_mouse, file_on_key);
                        } else if (start_action == 3) {
                            wm_create_window(160, 160, 400, 300, "Task Manager", taskmgr_on_draw);
                        } else if (start_action == 4) {
                            run_shutdown();
                        } else if (start_action == 8) {
                            outb(0x64, 0xFE); // Reboot
                        } else if (start_action == 9) {
                            logged_in = 0; // Lock
                        } else if (start_action == 10) {
                            __asm__ volatile("hlt"); // Sleep
                        } else if (start_action == 5) {
                            int w_calc = wm_create_window(180, 180, 200, 250, "Calculator", calc_on_draw);
                            wm_set_callbacks(w_calc, calc_on_mouse, 0);
                        } else if (start_action == 6) {
                            int w = wm_create_window(200, 200, 400, 300, "Notepad", notepad_on_draw);
                            wm_set_callbacks(w, notepad_on_mouse, notepad_on_key);
                        } else if (start_action == 12) {
                            int w = wm_create_window(300, 250, 300, 200, "Settings", settings_on_draw);
                            wm_set_callbacks(w, settings_on_mouse, 0);
                        } else if (start_action == 7) {
                            wm_create_window(220, 220, 300, 200, "This Computer", this_pc_on_draw);
                        } else if (start_action == 11) {
                            int w = wm_create_window(240, 240, 700, 500, "Firefox", firefox_on_draw);
                            void firefox_on_key(char c);
                            wm_set_callbacks(w, 0, firefox_on_key);
                        } else if (start_action == 13) {
                            extern void app_store_on_draw(int,int,int,int);
                            extern void app_store_on_mouse(int,int,int);
                            int w = wm_create_window(150, 150, 400, 300, "App Store", app_store_on_draw);
                            wm_set_callbacks(w, app_store_on_mouse, 0);
                        }
                        dirty = 1;
                    }
                    
                    if (m_btn) dirty = 1;
                }
            }
        }
        
        while (kbd_head != kbd_tail) {
            unsigned char sc = kbd_buf[kbd_tail];
            kbd_tail = (kbd_tail + 1) % KBD_BUF_SIZE;
            processed = 1;
            
            if (sc == 0x1D) ctrl_held = 1;
            else if (sc == 0x9D) ctrl_held = 0;
            if (sc == 0x5B) win_held = 1;
            else if (sc == 0xDB) win_held = 0;

            if(!(sc&0x80)){
                if (win_held && sc == 0x2F) { // 'V' make code
                    wm_create_window(cur_x, cur_y, 250, 150, "Win+V", win_v_on_draw);
                    dirty = 1;
                } else if (win_held && sc == 0x1F) { // 'S' make code
                    spotlight_active = 1;
                    spotlight_len = 0; spotlight_buf[0] = 0;
                    dirty = 1;
                } else if (ctrl_held && sc == 0x2E) { // 'C' make code
                    clipboard_head = (clipboard_head - 1 + 5) % 5;
                    extern void scopy(char*, const char*);
                    scopy(clipboard_ring[clipboard_head], "Copied from GUI!");
                } else {
                    char c=sc2ascii(sc);
                    if (c) {
                        if (!logged_in) {
                            login_error = 0;
                            if (c == '\t') login_focus = (login_focus + 1) % (users_count == 0 ? 3 : 2);
                            else if (c == '\n') {
                                if (users_count == 0) {
                                    int match = 1;
                                    for(int k=0; k<32; k++) { if (login_upass[k] != login_cpass[k]) match = 0; }
                                    if (login_p_len == 0 || login_u_len == 0 || !match) {
                                        login_error = 2;
                                    } else {
                                        auth_add_user(login_uname, login_upass);
                                        auth_login(login_uname, login_upass);
                                    }
                                }
                                else { 
                                    if (!auth_login(login_uname, login_upass)) login_error = 1;
                                    else { login_uname[0]=0; login_upass[0]=0; login_cpass[0]=0; login_u_len=0; login_p_len=0; login_c_len=0; }
                                }
                            } else if (c == '\b') {
                                if (login_focus == 0 && login_u_len > 0) login_uname[--login_u_len]=0;
                                if (login_focus == 1 && login_p_len > 0) login_upass[--login_p_len]=0;
                                if (login_focus == 2 && login_c_len > 0) login_cpass[--login_c_len]=0;
                            } else {
                                if (login_focus == 0 && login_u_len < 31) { login_uname[login_u_len++]=c; login_uname[login_u_len]=0; }
                                if (login_focus == 1 && login_p_len < 31) { login_upass[login_p_len++]=c; login_upass[login_p_len]=0; }
                                if (login_focus == 2 && login_c_len < 31) { login_cpass[login_c_len++]=c; login_cpass[login_c_len]=0; }
                            }
                            dirty = 1;
                        } else {
                            wm_handle_key(c); dirty = 1; 
                        }
                    }
                }
            }
        }
        
        process_schedule(); // Cooperative multitasking tick
        
        if (!processed && !dirty && cur_x == prev_x && cur_y == prev_y) {
            __asm__ volatile("cli");
            if (mouse_head == mouse_tail && kbd_head == kbd_tail) {
                __asm__ volatile("sti\n\thlt");
            } else {
                __asm__ volatile("sti");
            }
        }        
        if (dirty) {
            restore_mouse_bg(prev_x, prev_y);
            if (!logged_in) {
                draw_login(users_count == 0);
            } else {
                wm_render();
            }
            gfx_swap(); // Only swap 3MB on layout change
            save_mouse_bg(cur_x, cur_y);
            draw_mouse(cur_x, cur_y);
            prev_x = cur_x; prev_y = cur_y;
            dirty = 0;
        } else if (cur_x != prev_x || cur_y != prev_y) {
            restore_mouse_bg(prev_x, prev_y);
            save_mouse_bg(cur_x, cur_y);
            draw_mouse(cur_x, cur_y);
            prev_x = cur_x; prev_y = cur_y;
        }
    }
}
