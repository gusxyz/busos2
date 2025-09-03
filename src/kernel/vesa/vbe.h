#pragma once
#include <stdint.h>
#include "../multiboot.h"

#define FRAMEBUFFER_VIRTUAL_ADDR 0xE0000000
extern uint32_t scanline;

void vbeInit(struct multiboot_info *bootInfo);

void putchar(
    /* note that this is int, not char as it's a unicode character */
    uint16_t c,
    /* cursor position on screen, in characters not in pixels */
    int cx, int cy,
    /* foreground and background colors, say 0xFFFFFF and 0x000000 */
    uint32_t fg, uint32_t bg);
