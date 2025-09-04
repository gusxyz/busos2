#include "kernel.h"

uint32_t g_ebda_addr = 0;

void kernel_main(uint32_t magic, multiboot_info_t *bootInfo)
{
    serialInit();
    serial_putsf("Hello World!\n");
    // print("Booting Bus Kernel...\n");
    initGDT();
    initIDT();
    initTimer();

    if (bootInfo->flags & MULTIBOOT_INFO_MEM_MAP)
    {
        multiboot_mmap_entry_t *mmap = (multiboot_mmap_entry_t *)bootInfo->mmap_addr;

        while ((uint32_t)mmap < bootInfo->mmap_addr + bootInfo->mmap_length)
        {
            if (mmap->type == MULTIBOOT_MEMORY_RESERVED && mmap->addr_high == 0 && mmap->addr_low < 0x100000)
            {
                // Store the result in our global variable for later use
                g_ebda_addr = mmap->addr_low;
            }
            mmap = (multiboot_mmap_entry_t *)((uint32_t)mmap + mmap->size + sizeof(mmap->size));
        }
    }
    if (g_ebda_addr != 0)
    {
        serial_putsf("Found EBDA in memory map at 0x%x\n", g_ebda_addr);
    }

    // memory
    uint32_t mod1 = *(uint32_t *)(bootInfo->mods_addr);
    uint32_t physicalAllocStart = (mod1 + 0xFFF) & ~0XFFF;
    initMemory((uint32_t)bootInfo->mem_upper * 1024, physicalAllocStart);
    multiboot_info_t *vbi = (multiboot_info_t *)((uint32_t)bootInfo + KERNEL_START);

    // video
    vbeInit(bootInfo);
    consoleInit();

    // devices
    initKeyboard();
    pciInit(true);
    initIDEController();
    acpiInit(g_ebda_addr);
    rsdt_parse();
    // readAndParsePVD(0); // drive 0
    // printfileSystemTree(0); // dear god its jason borne
    consoleMarkInputStart();
    for (;;)
        ;
}
