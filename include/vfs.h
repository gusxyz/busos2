#ifndef VFS_H
#define VFS_H

struct filesystem_driver;

typedef struct mountpoint
{
    uint8_t drive;    // The physical device
    const char *path; // Where it's mounted (e.g., "/mnt/hdd")

    // The filesystem driver for this mount (e.g., the ext2 driver)
    struct filesystem_driver *driver;

    // The root node of this mounted filesystem
    // vfs_node_t *root_node;

    // Link to the next mountpoint in our global list
    struct mountpoint *next;
} mountpoint_t;

#endif