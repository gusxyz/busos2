#pragma once
#include <stdint.h>
#include <multiboot.h>

#define FRAMEBUFFER_VIRTUAL_ADDR 0xE0000000
extern uint32_t scanline;
extern uint32_t vbe_width;
extern uint32_t vbe_height;
void vbeInit(multiboot_info_t *bootInfo);