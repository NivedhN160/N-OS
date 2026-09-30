#include "pci.h"
#include "vga.h"

static inline void outl(unsigned short port, unsigned int data) {
    __asm__ volatile("outl %0, %1" : : "a"(data), "Nd"(port));
}
static inline unsigned int inl(unsigned short port) {
    unsigned int ret;
    __asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

unsigned short pci_config_read_word(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset) {
    unsigned int address;
    unsigned int lbus  = (unsigned int)bus;
    unsigned int lslot = (unsigned int)slot;
    unsigned int lfunc = (unsigned int)func;
    unsigned short tmp = 0;

    address = (unsigned int)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xfc) | ((unsigned int)0x80000000));
    outl(0xCF8, address);
    tmp = (unsigned short)((inl(0xCFC) >> ((offset & 2) * 8)) & 0xffff);
    return tmp;
}

int gpu_accelerated = 0;

void pci_init() {
    // Scan buses
    for(int bus = 0; bus < 256; bus++) {
        for(int slot = 0; slot < 32; slot++) {
            unsigned short vendor = pci_config_read_word(bus, slot, 0, 0);
            if(vendor != 0xFFFF) {
                unsigned short device = pci_config_read_word(bus, slot, 0, 2);
                unsigned short class_subclass = pci_config_read_word(bus, slot, 0, 10);
                
                // Class code is the upper byte of offset 10
                if ((class_subclass >> 8) == 0x03) {
                    gpu_accelerated = 1; // Found Display Controller (GPU)
                }

                if (vendor == 0x10EC && device == 0x8139) {
                    // Found RTL8139
                    void rtl8139_init(unsigned char, unsigned char, unsigned char);
                    rtl8139_init(bus, slot, 0);
                }
            }
        }
    }
}
