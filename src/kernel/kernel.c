#include "kernel.h"

void kernel_main(uint32_t magic, struct multiboot_info *bootInfo)
{
    reset();
    disableCursor();
    print("Booting Bus Kernel...\n");
    initGDT();
    initIDT();
    initTimer();
    initKeyboard();

    uint32_t mod1 = *(uint32_t *)(bootInfo->mods_addr + 4);
    uint32_t physicalAllocStart = (mod1 + 0xFFF) & ~0XFFF;
    initMemory((uint32_t)bootInfo->mem_upper * 1024, physicalAllocStart);
    pciInit(true);
    initIDEController();
    enableCursor(0, 0);

    for (;;)
        ;
}