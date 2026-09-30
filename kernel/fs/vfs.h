#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include "block.h"

#define FS_FILE        0x01
#define FS_DIRECTORY   0x02
#define FS_MOUNTPOINT  0x08

struct fs_node;

typedef struct fs_node {
    char name[64];
    uint32_t inode;
    uint32_t flags;
    uint32_t size;
    void *impl;
    struct fs_node *parent;
    struct fs_node *children;
    struct fs_node *next;
} fs_node_t;

void vfs_init(void);
int vfs_mount(const char *path, block_dev_t *dev, int fstype);
fs_node_t *vfs_open(const char *path, int flags);
int vfs_read(fs_node_t *node, void *buf, uint32_t size, uint32_t offset);
int vfs_write(fs_node_t *node, void *buf, uint32_t size, uint32_t offset);
int vfs_readdir(fs_node_t *node, uint32_t index, struct fs_node *out);
int vfs_close(fs_node_t *node);

#endif
