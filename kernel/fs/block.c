#include "block.h"
extern void ata_read_sectors(uint32_t lba, uint8_t sectors, uint8_t *buffer);

static void ata0_read(uint32_t lba, uint8_t *buf, uint32_t count) {
    ata_read_sectors(lba, count, buf);
}
static void ata0_write(uint32_t lba, uint8_t *buf, uint32_t count) {
    // stub
}

static block_dev_t ata0 = {
    .read = ata0_read,
    .write = ata0_write,
    .block_size = 512
};

block_dev_t *block_get_ata0(void) {
    return &ata0;
}
