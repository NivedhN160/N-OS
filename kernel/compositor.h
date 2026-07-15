#ifndef COMPOSITOR_H
#define COMPOSITOR_H
#include <stdint.h>

void compositor_init();
void compositor_render();
void compositor_request_redraw(int window_id);

#endif
