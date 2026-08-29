#include "../include/fs/fat32.h"
#include "../include/fs/vfs.h"
#include "../include/ata.h"
#include "../include/pmm.h" // For allocating a cluster buffer
#include <stddef.h>

extern void serial_write(const char *str);

static fat32_bpb_t bpb;
static uint32_t fat_start_lba;
static uint32_t data_start_lba;
static uint32_t root_cluster;
static vfs_node_t root_node;

static uint8_t *cluster_buffer; // Temporary buffer to read clusters

static uint32_t cluster_to_lba(uint32_t cluster) {
    return data_start_lba + ((cluster - 2) * bpb.sectors_per_cluster);
}

// Compare FAT name (11 chars padded with spaces) with standard string
static int compare_fat_name(const char *fat_name, const char *search_name) {
    char formatted[11];
    for (int i = 0; i < 11; i++) formatted[i] = ' ';
    
    int i = 0, j = 0;
    while (search_name[j] && search_name[j] != '.' && i < 8) {
        char c = search_name[j++];
        if (c >= 'a' && c <= 'z') c -= 32; // Uppercase
        formatted[i++] = c;
    }
    
    if (search_name[j] == '.') {
        j++;
        i = 8;
        while (search_name[j] && i < 11) {
            char c = search_name[j++];
            if (c >= 'a' && c <= 'z') c -= 32;
            formatted[i++] = c;
        }
    }
    
    for (int k = 0; k < 11; k++) {
        if (fat_name[k] != formatted[k]) return 0;
    }
    return 1;
}

static vfs_node_t *fat32_finddir(vfs_node_t *node, char *name) {
    uint32_t cluster = node->first_cluster;
    
    while (cluster < 0x0FFFFFF8) {
        uint32_t lba = cluster_to_lba(cluster);
        ata_read_sectors(lba, cluster_buffer, bpb.sectors_per_cluster);
        
        fat32_dir_t *dir = (fat32_dir_t *)cluster_buffer;
        int entries = (bpb.sectors_per_cluster * 512) / sizeof(fat32_dir_t);
        
        for (int i = 0; i < entries; i++) {
            if (dir[i].name[0] == 0x00) break; // End of directory
            if (dir[i].name[0] == 0xE5) continue; // Deleted
            
            if (dir[i].attr == 0x0F) continue; // Long File Name
            
            if (compare_fat_name(dir[i].name, name)) {
                // We don't have a kmalloc, so we'll just allocate a whole page for the node (wasteful, but works)
                vfs_node_t *found = (vfs_node_t *)pmm_alloc_page();
                // Copy name loosely
                int name_idx = 0;
                while (name[name_idx] && name_idx < 127) {
                    found->name[name_idx] = name[name_idx];
                    name_idx++;
                }
                found->name[name_idx] = 0;
                
                found->flags = (dir[i].attr & 0x10) ? FS_DIRECTORY : FS_FILE;
                found->length = dir[i].size;
                found->first_cluster = ((uint32_t)dir[i].cluster_hi << 16) | dir[i].cluster_lo;
                found->read = node->read; // Use same read function
                found->finddir = (found->flags & FS_DIRECTORY) ? fat32_finddir : NULL;
                return found;
            }
        }
        
        // For simplicity, we assume single-cluster directories in this basic driver
        break;
    }
    return NULL;
}

// Call callback(name, is_dir) for every entry in the root directory
void fat32_listdir(void (*callback)(const char *name, int is_dir)) {
    uint32_t cluster = root_cluster;
    while (cluster < 0x0FFFFFF8) {
        uint32_t lba = cluster_to_lba(cluster);
        ata_read_sectors(lba, cluster_buffer, bpb.sectors_per_cluster);
        fat32_dir_t *dir = (fat32_dir_t *)cluster_buffer;
        int entries = (bpb.sectors_per_cluster * 512) / sizeof(fat32_dir_t);
        for (int i = 0; i < entries; i++) {
            if (dir[i].name[0] == 0x00) return; // end of directory
            if ((uint8_t)dir[i].name[0] == 0xE5) continue; // deleted
            if (dir[i].attr == 0x0F) continue; // LFN entry
            if (dir[i].attr & 0x08) continue; // volume label
            // Build a readable name: "KERNEL  ELF" -> "KERNEL.ELF"
            char readable[13];
            int ri = 0;
            for (int k = 0; k < 8 && dir[i].name[k] != ' '; k++)
                readable[ri++] = dir[i].name[k] >= 'A' && dir[i].name[k] <= 'Z'
                                  ? dir[i].name[k] + 32 : dir[i].name[k];
            if (dir[i].name[8] != ' ') {
                readable[ri++] = '.';
                for (int k = 8; k < 11 && dir[i].name[k] != ' '; k++)
                    readable[ri++] = dir[i].name[k] >= 'A' && dir[i].name[k] <= 'Z'
                                      ? dir[i].name[k] + 32 : dir[i].name[k];
            }
            readable[ri] = '\0';
            int is_dir = (dir[i].attr & 0x10) ? 1 : 0;
            callback(readable, is_dir);
        }
        break; // single-cluster root for now
    }
}

static uint32_t cached_fat_sector_lba = 0xFFFFFFFF;
static uint8_t cached_fat_sector[512];
static uint32_t cached_cluster_num = 0xFFFFFFFF;

static uint32_t fat32_get_next_cluster(uint32_t cluster) {
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = fat_start_lba + (fat_offset / 512);
    uint32_t ent_offset = fat_offset % 512;
    
    if (cached_fat_sector_lba != fat_sector) {
        ata_read_sector(fat_sector, cached_fat_sector);
        cached_fat_sector_lba = fat_sector;
    }
    
    uint32_t next = *(uint32_t*)&cached_fat_sector[ent_offset];
    return next & 0x0FFFFFFF;
}

static uint32_t fat32_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    if (offset >= node->length) return 0;
    if (offset + size > node->length) size = node->length - offset;
    
    uint32_t cluster_size = bpb.sectors_per_cluster * 512;
    uint32_t current_cluster = node->first_cluster;
    
    // Skip clusters to reach the offset
    uint32_t skip_clusters = offset / cluster_size;
    for (uint32_t i = 0; i < skip_clusters; i++) {
        current_cluster = fat32_get_next_cluster(current_cluster);
        if (current_cluster >= 0x0FFFFFF8) return 0;
    }
    
    uint32_t cluster_offset = offset % cluster_size;
    uint32_t bytes_read = 0;
    
    while (bytes_read < size && current_cluster < 0x0FFFFFF8) {
        if (cached_cluster_num != current_cluster) {
            uint32_t lba = cluster_to_lba(current_cluster);
            ata_read_sectors(lba, cluster_buffer, bpb.sectors_per_cluster);
            cached_cluster_num = current_cluster;
        }
        
        uint32_t to_read = cluster_size - cluster_offset;
        if (to_read > size - bytes_read) to_read = size - bytes_read;
        
        for (uint32_t i = 0; i < to_read; i++) {
            buffer[bytes_read + i] = cluster_buffer[cluster_offset + i];
        }
        
        bytes_read += to_read;
        cluster_offset = 0; // Only the first cluster has an offset
        current_cluster = fat32_get_next_cluster(current_cluster);
    }
    
    return bytes_read;
}

void fat32_init(void) {
    serial_write("FAT32: Initializing...\r\n");
    
    uint8_t boot_sector[512];
    ata_read_sector(0, boot_sector);
    
    for (int i = 0; i < sizeof(fat32_bpb_t); i++) {
        ((uint8_t*)&bpb)[i] = boot_sector[i];
    }
    
    if (bpb.bytes_per_sector != 512) {
        serial_write("FAT32: Invalid bytes per sector!\r\n");
        return;
    }
    
    fat_start_lba = bpb.reserved_sectors;
    data_start_lba = fat_start_lba + (bpb.fat_count * bpb.fat_size_32);
    root_cluster = bpb.root_cluster;
    
    // Allocate cluster buffer
    cluster_buffer = (uint8_t *)pmm_alloc_page(); // Max 4KB cluster support here
    
    // Setup root node
    root_node.flags = FS_DIRECTORY;
    root_node.first_cluster = root_cluster;
    root_node.finddir = fat32_finddir;
    root_node.read = fat32_read;
    
    fs_root = &root_node;
    
    serial_write("FAT32: Mounted successfully!\r\n");
}
