#ifndef TCP_H
#define TCP_H

#include <stdint.h>

typedef enum { TCP_CLOSED, TCP_SYN_SENT, TCP_ESTABLISHED, TCP_FIN_WAIT1 } tcp_state_t;

typedef struct tcp_pcb {
    tcp_state_t state;
    uint32_t local_ip, remote_ip;
    uint16_t local_port, remote_port;
    uint32_t snd_una, snd_nxt, rcv_nxt;
    uint8_t *rx_buf; 
    uint32_t rx_len;
} tcp_pcb_t;

void tcp_init(void);
int  tcp_connect(tcp_pcb_t *pcb, uint32_t dest_ip, uint16_t port);
int  tcp_send(tcp_pcb_t *pcb, const void *data, int len);
int  tcp_recv(tcp_pcb_t *pcb, void *buf, int maxlen);
void tcp_close(tcp_pcb_t *pcb);
void tcp_timer_tick(void);

#endif
