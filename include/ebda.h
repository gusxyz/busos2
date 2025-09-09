#ifndef EBDA_H
#define EBDA_H

#include <stdint.h>
#include <multiboot.h>

uint32_t getEBDA(multiboot_info_t *bootInfo);

#endif