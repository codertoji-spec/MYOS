#include "../include/fs/vfs.h"
#include <stddef.h>

vfs_node_t *fs_root = NULL;

void vfs_init(void) {
    fs_root = NULL;
}

uint32_t vfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    if (node->read) {
        return node->read(node, offset, size, buffer);
    }
    return 0;
}

vfs_node_t *vfs_finddir(vfs_node_t *node, char *name) {
    if ((node->flags & FS_DIRECTORY) && node->finddir) {
        return node->finddir(node, name);
    }
    return NULL;
}

static int streq(const char *a, const char *b) {
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

vfs_node_t *vfs_open(const char *path) {
    if (!fs_root) return NULL;
    
    // Simplistic: only support root directory files, e.g. "kernel.elf"
    // Since this is a basic OS, parsing nested paths (a/b/c) is omitted for brevity.
    
    // Skip leading slash if present
    if (path[0] == '/') {
        path++;
    }
    
    return vfs_finddir(fs_root, (char*)path);
}
