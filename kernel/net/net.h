#ifndef NET_H
#define NET_H

// OSI Layer 2: Data Link (Ethernet)
typedef struct {
    unsigned char dest_mac[6];
    unsigned char src_mac[6];
    unsigned short ethertype;
} __attribute__((packed)) ethernet_frame_t;

// OSI Layer 3: Network (IPv4)
typedef struct {
    unsigned char ihl : 4;
    unsigned char version : 4;
    unsigned char dscp;
    unsigned short length;
    unsigned short id;
    unsigned short flags_offset;
    unsigned char ttl;
    unsigned char protocol;
    unsigned short checksum;
    unsigned int src_ip;
    unsigned int dest_ip;
} __attribute__((packed)) ipv4_packet_t;

// OSI Layer 4: Transport (TCP/UDP)
typedef struct {
    unsigned short src_port;
    unsigned short dest_port;
    unsigned int seq_num;
    unsigned int ack_num;
    unsigned short flags;
    unsigned short window_size;
    unsigned short checksum;
    unsigned short urgent_ptr;
} __attribute__((packed)) tcp_segment_t;

// OSI Layer 7: Application (HTTP/DNS stubs)
void net_init();
void net_send_http_request(const char *url);
void net_handle_packet(unsigned char *data, int len);
void net_send_ping(unsigned int dest_ip);

// ARP
typedef struct {
    unsigned short htype;
    unsigned short ptype;
    unsigned char hlen;
    unsigned char plen;
    unsigned short opcode;
    unsigned char sender_mac[6];
    unsigned int sender_ip;
    unsigned char target_mac[6];
    unsigned int target_ip;
} __attribute__((packed)) arp_packet_t;

// ICMP
typedef struct {
    unsigned char type;
    unsigned char code;
    unsigned short checksum;
    unsigned short id;
    unsigned short seq;
} __attribute__((packed)) icmp_packet_t;

#define AF_INET 2
#define SOCK_STREAM 1
#define IPPROTO_TCP 6

struct sockaddr_in {
    short sin_family;
    unsigned short sin_port;
    unsigned int sin_addr;
};

typedef enum {
    SOCK_CLOSED = 0,
    SOCK_SYN_SENT,
    SOCK_ESTABLISHED,
    SOCK_FIN_WAIT
} tcp_state_t;

typedef struct {
    int id;
    int domain;
    int type;
    int protocol;
    tcp_state_t state;
    unsigned int local_ip;
    unsigned short local_port;
    unsigned int remote_ip;
    unsigned short remote_port;
    unsigned int seq_num;
    unsigned int ack_num;
} socket_t;

int socket(int domain, int type, int protocol);
int connect(int sockfd, const struct sockaddr_in *addr);
int send(int sockfd, const void *buf, int len, int flags);
int recv(int sockfd, void *buf, int len, int flags);
void close_socket(int sockfd);

#endif
