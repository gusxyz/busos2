#include <stdio.h>
#include <liballoc.h>
#include <stdio.h>
#include <gdt.h>
#include <idt.h>
#include <timer.h>
#include <multiboot.h>
#include <memory.h>
#include <keyboard.h>
#include <drivers/pci.h>
#include <drivers/ide.h>
#include <filesystems.h>
#include <vbe.h>
#include <console.h>
#include <rsdp.h>
#include <scheduler/scheduler.h>
#include <ebda.h>

void kernel_main(uint32_t magic, multiboot_info_t *bootInfo)
{
    // basics
    serialInit();
    initGDT();
    initIDT();
    initTimer();

    // ebda
    uint32_t g_ebda_addr = getEBDA(bootInfo);

    // memory
    uint32_t mod1 = *(uint32_t *)(bootInfo->mods_addr);
    uint32_t physicalAllocStart = (mod1 + 0xFFF) & ~0XFFF;
    initMemory((uint32_t)bootInfo->mem_upper * 1024, physicalAllocStart);
    multiboot_info_t *vbi = (multiboot_info_t *)((uint32_t)bootInfo + KERNEL_START);

    // video
    vbeInit(bootInfo);
    consoleInit();

    // devices
    acpiInit(g_ebda_addr);
    pciInit(false);
    rsdt_parse();
    scheduler_init();
    ide_device_t *ideDevices = initIDEController();

    initKeyboard();

    printf("Kernel Booted in %ims\n", ticks);
    consoleMarkInputStart();

    asm volatile("sti");
    for (;;)
        asm volatile("hlt"); // <<<<<<<<<<< CHANGE THE LOOP TO THIS
}

// holy fuck i need to figure out what posix requires that shit is going to melt my fucking brain for sure