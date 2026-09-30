#include "socket.h"
#include "tcp.h"
extern void term_print(const char *s, unsigned int col);

void socket_init(void) {
    term_print("Socket API initialized.\n", 0x00FF00);
}
