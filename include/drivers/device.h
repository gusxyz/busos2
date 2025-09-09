#ifndef DEVICE_H
#define DEVICE_H

struct block_device;

typedef struct block_device
{
    char name[32];
    ide_device_t *ideDevicePtr;
    block_device_ops_t *ops;
} block_device_t

#endif