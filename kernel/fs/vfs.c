#include "vfs.h"
#include "../mm/kmalloc.h"

extern void term_print(const char *s, unsigned int col);

static fs_node_t *root = 0;

void vfs_init(void) {
    root = (fs_node_t*)kmalloc(sizeof(fs_node_t));
    if (!root) return;
    root->name[0] = '/';
    root->name[1] = 0;
    root->flags = FS_DIRECTORY | FS_MOUNTPOINT;
    root->parent = 0;
    root->children = 0;
    root->next = 0;
    term_print("VFS Initialized.\n", 0x00FF00);
}

int vfs_mount(const char *path, block_dev_t *dev, int fstype) {
    return 0; // Stub for now
}

fs_node_t *vfs_open(const char *path, int flags) {
    // Simple mock open for now
    return root;
}

int vfs_read(fs_node_t *node, void *buf, uint32_t size, uint32_t offset) {
    return 0;
}

int vfs_write(fs_node_t *node, void *buf, uint32_t size, uint32_t offset) {
    return 0;
}

int vfs_readdir(fs_node_t *node, uint32_t index, struct fs_node *out) {
    return -1;
}

int vfs_close(fs_node_t *node) {
    return 0;
}

