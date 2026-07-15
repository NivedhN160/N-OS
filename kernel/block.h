#ifndef BLOCK_H
#define BLOCK_H
#include <stdint.h>

typedef struct {
    int id;
    uint32_t sector_size;
    uint32_t total_sectors;
    int (*read_blocks)(int drive, uint32_t lba, uint8_t count, void *buffer);
    int (*write_blocks)(int drive, uint32_t lba, uint8_t count, void *buffer);
} block_device_t;

void block_register(block_device_t *dev);
int block_read(int drive, uint32_t lba, uint8_t count, void *buffer);
int block_write(int drive, uint32_t lba, uint8_t count, void *buffer);

#endif
