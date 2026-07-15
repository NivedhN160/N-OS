#ifndef VFS_H
#define VFS_H
#include <stdint.h>

typedef struct {
    char name[32];
    int size;
    int (*read)(char *path, void *buffer, int size);
    int (*write)(char *path, void *buffer, int size);
} fs_node_t;

void vfs_init();
int vfs_read(char *path, void *buffer, int size);
int vfs_write(char *path, void *buffer, int size);

#endif
