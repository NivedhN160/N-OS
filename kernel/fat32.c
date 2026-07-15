#include "fat32.h"
#include "block.h"
#include "kmalloc.h"

typedef struct {
    uint8_t jump[3];
    char oem[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_count;
    uint16_t dir_entries;
    uint16_t total_sectors_16;
    uint8_t media_descriptor;
    uint16_t fat_size_16;
    uint16_t sectors_per_track;
    uint16_t heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint32_t fat_size_32;
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;
} __attribute__((packed)) fat32_bs_t;

void fat32_init(int drive) {
    uint8_t *boot_sector = kmalloc(512);
    block_read(drive, 0, 1, boot_sector);
    
    // Parse BS
    fat32_bs_t *bs = (fat32_bs_t*)boot_sector;
    if (bs->bytes_per_sector == 512) {
        extern void term_print(const char*, unsigned int);
        term_print("FAT32 Volume detected.\n", 0x00FF00);
    }
    kfree(boot_sector);
}

int fat32_read_file(const char* filename, void* buffer) {
    (void)filename; (void)buffer;
    return 0; // Stub
}

int fat32_write_file(const char* filename, void* buffer, int size) {
    (void)filename; (void)buffer; (void)size;
    return 0; // Stub
}
