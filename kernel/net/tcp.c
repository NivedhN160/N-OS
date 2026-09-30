#include "tcp.h"

extern void term_print(const char *s, unsigned int col);

void tcp_init(void) {
    term_print("TCP initialized.\n", 0x00FF00);
}
int tcp_connect(tcp_pcb_t *pcb, uint32_t dest_ip, uint16_t port) { return 0; }
int tcp_send(tcp_pcb_t *pcb, const void *data, int len) { return len; }
int tcp_recv(tcp_pcb_t *pcb, void *buf, int maxlen) { return 0; }
void tcp_close(tcp_pcb_t *pcb) { pcb->state = TCP_CLOSED; }
void tcp_timer_tick(void) {}
