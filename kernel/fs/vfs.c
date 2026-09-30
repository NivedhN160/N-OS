#include "vfs.h"

#define MAX_NODES 32
static fs_node_t nodes[MAX_NODES];
static int node_count = 0;

void vfs_init() {
    node_count = 0;
}

int vfs_read(char *path, void *buffer, int size) {
    (void)path; (void)buffer; (void)size;
    // Walk VFS tree and dispatch
    return -1; 
}

int vfs_write(char *path, void *buffer, int size) {
    (void)path; (void)buffer; (void)size;
    // Walk VFS tree and dispatch
    return -1;
}
