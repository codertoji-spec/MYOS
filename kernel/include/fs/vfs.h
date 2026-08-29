#pragma once

#include <stdint.h>
#include <stddef.h>

#define FS_FILE        0x01
#define FS_DIRECTORY   0x02

struct vfs_node;

typedef uint32_t (*read_type_t)(struct vfs_node*, uint32_t, uint32_t, uint8_t*);
typedef struct vfs_node* (*finddir_type_t)(struct vfs_node*, char* name);

typedef struct vfs_node {
    char name[128];
    uint32_t flags;
    uint32_t length;
    uint32_t first_cluster; // Specific to FAT32 but useful
    
    read_type_t read;
    finddir_type_t finddir;
    
    struct vfs_node *ptr; // Used by driver to track node specific data
} vfs_node_t;

extern vfs_node_t *fs_root;

void vfs_init(void);
uint32_t vfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
vfs_node_t *vfs_finddir(vfs_node_t *node, char *name);
vfs_node_t *vfs_open(const char *path);
