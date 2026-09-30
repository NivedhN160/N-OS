#ifndef SECURITY_H
#define SECURITY_H

#include "net.h"

// Define max firewall rules
#define MAX_FW_RULES 32

typedef struct {
    unsigned int src_ip;
    unsigned int src_mask;
    unsigned short dest_port;
    unsigned char protocol; // 6=TCP, 17=UDP, 0=Any
    unsigned char action; // 0=DROP, 1=ACCEPT
} fw_rule_t;

void security_init();
void security_add_rule(unsigned int src_ip, unsigned int src_mask, unsigned short dest_port, unsigned char protocol, unsigned char action);
int security_check_packet(ipv4_packet_t *ip_pkt, int len);

#endif
