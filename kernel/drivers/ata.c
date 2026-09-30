#include "ata.h"
#include "block.h"

static inline void outb(unsigned short port, unsigned char val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline unsigned char inb(unsigned short port) {
    unsigned char ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void insw(uint16_t port, void *addr, uint32_t count) {
    __asm__ volatile("cld; rep insw" : "+D"(addr), "+c"(count) : "d"(port) : "memory");
}

static inline void outsw(uint16_t port, void *addr, uint32_t count) {
    __asm__ volatile("cld; rep outsw" : "+S"(addr), "+c"(count) : "d"(port) : "memory");
}

static void ata_wait() {
    for(int i=0; i<4; i++) inb(0x1F7);
}

static int ata_read_sector(int drive, uint32_t lba, uint8_t count, void *buffer) {
    (void)drive;
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F2, count);
    outb(0x1F3, (uint8_t) lba);
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F7, 0x20); // READ SECTORS
    
    for (int i=0; i<count; i++) {
        while (!(inb(0x1F7) & 0x08));
        insw(0x1F0, (uint8_t*)buffer + i * 512, 256);
    }
    return 0;
}

static int ata_write_sector(int drive, uint32_t lba, uint8_t count, void *buffer) {
    (void)drive;
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F2, count);
    outb(0x1F3, (uint8_t) lba);
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F7, 0x30); // WRITE SECTORS
    
    for (int i=0; i<count; i++) {
        while (!(inb(0x1F7) & 0x08));
        outsw(0x1F0, (uint8_t*)buffer + i * 512, 256);
        outb(0x1F7, 0xE7); // Cache flush
        while (inb(0x1F7) & 0x80);
    }
    return 0;
}

static block_device_t ata_dev = {
    .sector_size = 512,
    .total_sectors = 0, // Should read from IDENTIFY
    .read_blocks = ata_read_sector,
    .write_blocks = ata_write_sector
};

void ata_init() {
    block_register(&ata_dev);
}
