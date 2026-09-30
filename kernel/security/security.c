#include "security.h"
#include "vga.h" // for drawing alerts

static fw_rule_t rules[MAX_FW_RULES];
static int rule_count = 0;
static int dropped_packets = 0;

void security_init() {
    rule_count = 0;
    dropped_packets = 0;
    // Default rule: Drop incoming connections to port 23 (Telnet)
    security_add_rule(0x00000000, 0x00000000, 23, 6, 0); 
}

void security_add_rule(unsigned int src_ip, unsigned int src_mask, unsigned short dest_port, unsigned char protocol, unsigned char action) {
    if (rule_count < MAX_FW_RULES) {
        rules[rule_count].src_ip = src_ip;
        rules[rule_count].src_mask = src_mask;
        rules[rule_count].dest_port = dest_port;
        rules[rule_count].protocol = protocol;
        rules[rule_count].action = action;
        rule_count++;
    }
}

int security_check_packet(ipv4_packet_t *ip_pkt, int len) {
    // 0 = Drop, 1 = Accept
    unsigned int src_ip = ip_pkt->src_ip;
    unsigned char proto = ip_pkt->protocol;
    
    // Extract dest port if TCP or UDP
    unsigned short dest_port = 0;
    if (proto == 6) { // TCP
        tcp_segment_t *tcp = (tcp_segment_t*)((unsigned char*)ip_pkt + 20); // assuming no IP options
        dest_port = ((tcp->dest_port & 0xFF00) >> 8) | ((tcp->dest_port & 0x00FF) << 8); // ntohs
    }
    
    for (int i=0; i<rule_count; i++) {
        if (rules[i].protocol != 0 && rules[i].protocol != proto) continue;
        if (rules[i].dest_port != 0 && rules[i].dest_port != dest_port) continue;
        
        if ((src_ip & rules[i].src_mask) == (rules[i].src_ip & rules[i].src_mask)) {
            if (rules[i].action == 0) {
                dropped_packets++;
                // Alert in kernel!
                // term_print("FIREWALL: Dropped malicious packet!\n", 0xFF0000);
                return 0; // Drop
            } else {
                return 1; // Accept
            }
        }
    }
    
    return 1; // Default Accept
}
