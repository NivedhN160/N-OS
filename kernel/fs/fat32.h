#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>
#include "vfs.h"

void fat32_init(uint32_t partition_offset);

#endif
