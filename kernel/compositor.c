#include "compositor.h"
#include <stdint.h>

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768

// Off-screen buffer for compositor to eliminate tearing
uint32_t backbuffer[SCREEN_WIDTH * SCREEN_HEIGHT];

void compositor_init() {
    // Clear backbuffer
    for (int i=0; i<SCREEN_WIDTH*SCREEN_HEIGHT; i++) {
        backbuffer[i] = 0x000000;
    }
}

void compositor_render() {
    // In a real implementation, this blends window buffers back-to-front
    // For now, it simply swaps the backbuffer to the front buffer
    extern void gfx_swap();
    gfx_swap();
}

void compositor_request_redraw(int window_id) {
    (void)window_id;
    // Mark region as dirty, but we just trigger full render for now
}
