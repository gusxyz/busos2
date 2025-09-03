#include "kernel.h"

void kernel_main(uint32_t magic, struct multiboot_info *bootInfo)
{
    reset();
    disableCursor();
    // print("Booting Bus Kernel...\n");
    initGDT();
    initIDT();
    initTimer();
    uint32_t mod1 = *(uint32_t *)(bootInfo->mods_addr);
    uint32_t physicalAllocStart = (mod1 + 0xFFF) & ~0XFFF;
    initMemory((uint32_t)bootInfo->mem_upper * 1024, physicalAllocStart);
    vbeInit(bootInfo);
    initKeyboard();
    // tty_initialize(FRAMEBUFFER_VIRTUAL_ADDR, width, height, ...);
    pciInit(true);
    initIDEController();
    // readAndParsePVD(0); // drive 0
    // // printfileSystemTree(0); // dear god its jason borne
    enableCursor(0, 0);
    for (;;)
        ;
}