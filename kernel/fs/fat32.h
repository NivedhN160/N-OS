#ifndef FAT32_H
#define FAT32_H
#include <stdint.h>

void fat32_init(int drive);
int fat32_read_file(const char* filename, void* buffer);
int fat32_write_file(const char* filename, void* buffer, int size);

#endif
