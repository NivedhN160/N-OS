#include "rtl8139.h"
#include "pci.h"

static unsigned int rtl_io_base = 0;
static unsigned char mac_addr[6];

static inline void outb(unsigned short port, unsigned char data) {
    __asm__ volatile("outb %0, %1" : : "a"(data), "Nd"(port));
}
static inline unsigned char inb(unsigned short port) {
    unsigned char ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void rtl8139_init(unsigned char bus, unsigned char slot, unsigned char func) {
    // Read BAR0 to get I/O base
    unsigned int bar0 = pci_config_read_word(bus, slot, func, 0x10) | 
                       (pci_config_read_word(bus, slot, func, 0x12) << 16);
    rtl_io_base = bar0 & ~3;
    
    // Enable PCI Bus Mastering
    // (Omitted standard PCI write for simplicity of mock)
    
    // Turn on the RTL8139
    outb(rtl_io_base + 0x52, 0x0);
    
    // Software Reset
    outb(rtl_io_base + 0x37, 0x10);
    while((inb(rtl_io_base + 0x37) & 0x10) != 0) {
        // Wait for reset
    }
    
    // Read MAC address
    for(int i = 0; i < 6; i++) {
        mac_addr[i] = inb(rtl_io_base + i);
    }
}

void rtl8139_send_packet(void *data, int len) {
    // Real driver would copy to TX buffer and trigger DMA
    // outl(rtl_io_base + 0x20, tx_phys_addr);
    // outl(rtl_io_base + 0x10, len);
}
