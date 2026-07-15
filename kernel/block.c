#include "block.h"

#define MAX_BLOCK_DEVICES 8
static block_device_t* devices[MAX_BLOCK_DEVICES];
static int dev_count = 0;

void block_register(block_device_t *dev) {
    if (dev_count < MAX_BLOCK_DEVICES) {
        dev->id = dev_count;
        devices[dev_count++] = dev;
    }
}

int block_read(int drive, uint32_t lba, uint8_t count, void *buffer) {
    if (drive < 0 || drive >= dev_count || !devices[drive]->read_blocks) return -1;
    return devices[drive]->read_blocks(drive, lba, count, buffer);
}

int block_write(int drive, uint32_t lba, uint8_t count, void *buffer) {
    if (drive < 0 || drive >= dev_count || !devices[drive]->write_blocks) return -1;
    return devices[drive]->write_blocks(drive, lba, count, buffer);
}
