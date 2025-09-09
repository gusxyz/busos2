#ifndef VFS_H
#define VFS_H

struct filesystem_driver_s;
struct mountpoint_s;
struct vfs_node_s;

typedef struct vfs_node_s
{

    void *privateData; // points to lower level fs
} vfs_node_t;

typedef struct mountpoint_s
{
    uint8_t drive;    // The physical device
    const char *path; // Where it's mounted (e.g., "/mnt/hdd")

    // The filesystem driver for this mount (e.g., the ext2 driver)
    struct filesystem_driver *driver;

    // The root node of this mounted filesystem
    vfs_node_t *root_node;

    // Link to the next mountpoint in our global list
    mountpoint_t *next;
} mountpoint_t;

#endif