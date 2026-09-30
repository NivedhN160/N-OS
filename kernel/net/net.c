#include "net.h"
#include "rtl8139.h"
#include "vga.h"

static unsigned int my_ip = 0x0A00020F; // 10.0.2.15 (VirtualBox Default)
extern void term_print(const char *str, unsigned int color);

void net_init() {
    // Initialized by PCI driver
}

void net_handle_packet(unsigned char *data, int len) {
    if (len < sizeof(ethernet_frame_t)) return;
    
    ethernet_frame_t *eth = (ethernet_frame_t *)data;
    unsigned short type = ((eth->ethertype & 0xFF) << 8) | (eth->ethertype >> 8); // ntohs
    
    if (type == 0x0806) { // ARP
        arp_packet_t *arp = (arp_packet_t *)(data + sizeof(ethernet_frame_t));
        if (arp->opcode == 0x0100) { // ARP Request (ntohs(1))
            term_print("NET: Received ARP Request\n", 0x00FF00);
            // We would send ARP Reply here
        }
    } else if (type == 0x0800) { // IPv4
        ipv4_packet_t *ip = (ipv4_packet_t *)(data + sizeof(ethernet_frame_t));
        
        // Pass through firewall
        int security_check_packet(ipv4_packet_t *, int);
        if (!security_check_packet(ip, len)) return;
        
        if (ip->protocol == 1) { // ICMP
            icmp_packet_t *icmp = (icmp_packet_t *)((unsigned char *)ip + (ip->ihl * 4));
            if (icmp->type == 8) { // Echo Request
                term_print("NET: Received PING (ICMP Echo Request)\n", 0x00FFFF);
                // We would send ICMP Echo Reply here
            } else if (icmp->type == 0) {
                term_print("NET: Received PING REPLY!\n", 0x00FF00);
            }
        } else if (ip->protocol == 6) { // TCP
            tcp_segment_t *tcp = (tcp_segment_t *)((unsigned char *)ip + (ip->ihl * 4));
            extern void net_tcp_handle(tcp_segment_t *, int);
            net_tcp_handle(tcp, len - sizeof(ethernet_frame_t) - (ip->ihl * 4));
        }
    }
}

void net_send_ping(unsigned int dest_ip) {
    term_print("NET: Sending PING...\n", 0xFFFF00);
    // In a real OS, we build the Ethernet -> IP -> ICMP layers
    // and call rtl8139_send_packet()
    unsigned char mock_packet[64] = {0};
    rtl8139_send_packet(mock_packet, 64);
}

// --- TCP State Machine ---
int tcp_state = 0; // 0 = CLOSED, 1 = SYN_SENT, 2 = ESTABLISHED
unsigned int tcp_seq = 1000;
unsigned int tcp_ack = 0;
unsigned short tcp_src_port = 49152;
unsigned short tcp_dest_port = 80;
unsigned int tcp_dest_ip = 0x8EFA448E; // 142.250.68.142 (google.com)
unsigned char router_mac[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56}; // QEMU default gateway MAC
unsigned char my_mac[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x57};

unsigned short calculate_checksum(unsigned short *ptr, int bytes) {
    long sum = 0;
    while (bytes > 1) { sum += *ptr++; bytes -= 2; }
    if (bytes > 0) sum += *((unsigned char*)ptr);
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return ~sum;
}

void net_send_tcp(unsigned short flags, const char *payload, int payload_len) {
    unsigned char packet[1500] = {0};
    ethernet_frame_t *eth = (ethernet_frame_t*)packet;
    ipv4_packet_t *ip = (ipv4_packet_t*)(packet + sizeof(ethernet_frame_t));
    tcp_segment_t *tcp = (tcp_segment_t*)(packet + sizeof(ethernet_frame_t) + sizeof(ipv4_packet_t));
    
    for(int i=0; i<6; i++) { eth->dest_mac[i] = router_mac[i]; eth->src_mac[i] = my_mac[i]; }
    eth->ethertype = 0x0008; // ntohs(0x0800)
    
    ip->ihl = 5; ip->version = 4;
    ip->length = ((sizeof(ipv4_packet_t) + sizeof(tcp_segment_t) + payload_len) << 8) | ((sizeof(ipv4_packet_t) + sizeof(tcp_segment_t) + payload_len) >> 8);
    ip->ttl = 64; ip->protocol = 6; // TCP
    ip->src_ip = my_ip; ip->dest_ip = tcp_dest_ip;
    ip->checksum = calculate_checksum((unsigned short*)(void*)ip, sizeof(ipv4_packet_t));
    
    tcp->src_port = (tcp_src_port << 8) | (tcp_src_port >> 8);
    tcp->dest_port = (tcp_dest_port << 8) | (tcp_dest_port >> 8);
    tcp->seq_num = ((tcp_seq & 0xFF) << 24) | ((tcp_seq & 0xFF00) << 8) | ((tcp_seq & 0xFF0000) >> 8) | ((tcp_seq >> 24) & 0xFF);
    tcp->ack_num = ((tcp_ack & 0xFF) << 24) | ((tcp_ack & 0xFF00) << 8) | ((tcp_ack & 0xFF0000) >> 8) | ((tcp_ack >> 24) & 0xFF);
    tcp->flags = (5 << 12) | flags; // Data offset 5
    tcp->window_size = 0xFFFF;
    
    // Copy payload
    if (payload_len > 0) {
        char *data = (char*)(packet + sizeof(ethernet_frame_t) + sizeof(ipv4_packet_t) + sizeof(tcp_segment_t));
        for(int i=0; i<payload_len; i++) data[i] = payload[i];
    }
    
    // Pseudo header for TCP checksum (Mock implementation)
    tcp->checksum = 0; // Skip checksum calculation for simplicity in mock
    
    rtl8139_send_packet(packet, sizeof(ethernet_frame_t) + sizeof(ipv4_packet_t) + sizeof(tcp_segment_t) + payload_len);
}

void net_send_http_request(const char *url) {
    term_print("NET: TCP SYN to Port 80...\n", 0xAAAAFF);
    tcp_state = 1; // SYN_SENT
    net_send_tcp(0x02, 0, 0); // SYN
}

char http_response_buffer[1024];

void net_tcp_handle(tcp_segment_t *tcp, int len) {
    unsigned short flags = tcp->flags & 0xFF; // simplified
    if (tcp_state == 1 && (flags & 0x12) == 0x12) { // SYN-ACK
        term_print("NET: TCP SYN-ACK Received.\n", 0x00FF00);
        tcp_ack = ((tcp->seq_num & 0xFF) << 24) | ((tcp->seq_num & 0xFF00) << 8) | ((tcp->seq_num & 0xFF0000) >> 8) | ((tcp->seq_num >> 24) & 0xFF) + 1;
        tcp_seq++;
        net_send_tcp(0x10, 0, 0); // ACK
        
        term_print("NET: Sending HTTP GET...\n", 0xAAAAFF);
        const char *req = "GET / HTTP/1.1\r\nHost: google.com\r\n\r\n";
        extern int slen(const char*);
        net_send_tcp(0x18, req, slen(req)); // PSH-ACK
        tcp_state = 2; // ESTABLISHED
    } else if (tcp_state == 2 && len > sizeof(tcp_segment_t)) {
        term_print("NET: HTTP Data Received!\n", 0x00FF00);
        char *payload = (char*)tcp + ((tcp->flags >> 12) * 4);
        extern int firefox_state;
        for (int i=0; i<1023 && i<(len - sizeof(tcp_segment_t)); i++) {
            http_response_buffer[i] = payload[i];
        }
        http_response_buffer[1023] = 0;
        firefox_state = 2; // Signal UI to render
    }
}

// ==========================================
// POSIX Sockets
// ==========================================
#define MAX_SOCKETS 32
static socket_t sockets[MAX_SOCKETS];
static int next_ephemeral_port = 49152;

int socket(int domain, int type, int protocol) {
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i].state == SOCK_CLOSED) {
            sockets[i].id = i;
            sockets[i].domain = domain;
            sockets[i].type = type;
            sockets[i].protocol = protocol;
            sockets[i].state = SOCK_CLOSED;
            sockets[i].local_port = next_ephemeral_port++;
            sockets[i].local_ip = 0x0A00020F; // 10.0.2.15
            return i;
        }
    }
    return -1;
}

int connect(int sockfd, const struct sockaddr_in *addr) {
    if (sockfd < 0 || sockfd >= MAX_SOCKETS) return -1;
    socket_t *sock = &sockets[sockfd];
    
    sock->remote_ip = addr->sin_addr;
    sock->remote_port = addr->sin_port;
    
    sock->state = SOCK_SYN_SENT;
    // Real implementation would send SYN packet here and wait for SYN-ACK.
    // Since net_send_tcp handles static routing, we fake the state transition for the mock API.
    sock->state = SOCK_ESTABLISHED;
    return 0;
}

int send(int sockfd, const void *buf, int len, int flags) {
    if (sockfd < 0 || sockfd >= MAX_SOCKETS) return -1;
    socket_t *sock = &sockets[sockfd];
    if (sock->state != SOCK_ESTABLISHED) return -1;
    
    // In a real OS, we build the IP/TCP headers here dynamically.
    // For this prototype, we'll bypass actual packet sending.
    sock->seq_num += len;
    
    return len;
}

int recv(int sockfd, void *buf, int len, int flags) {
    // Stub implementation for now (needs blocking queue)
    return 0; 
}

void close_socket(int sockfd) {
    if (sockfd < 0 || sockfd >= MAX_SOCKETS) return;
    sockets[sockfd].state = SOCK_CLOSED;
}
