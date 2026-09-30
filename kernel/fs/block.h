#ifndef BLOCK_H
#define BLOCK_H
#include <stdint.h>

typedef struct {
    void (*read)(uint32_t lba, uint8_t *buf, uint32_t count);
    void (*write)(uint32_t lba, uint8_t *buf, uint32_t count);
    uint32_t block_size;
} block_dev_t;

block_dev_t *block_get_ata0(void);

#endif
