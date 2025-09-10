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
    serial_init();
    init_gdt();
    init_idt();
    init_timer();

    // ebda
    uint32_t g_ebda_addr = getEBDA(bootInfo);

    // memory
    uint32_t mod1 = *(uint32_t *)(bootInfo->mods_addr);
    uint32_t physicalAllocStart = (mod1 + 0xFFF) & ~0XFFF;
    init_memory((uint32_t)bootInfo->mem_upper * 1024, physicalAllocStart);
    multiboot_info_t *vbi = (multiboot_info_t *)((uint32_t)bootInfo + KERNEL_START);

    // video
    vbe_init(bootInfo);
    console_init();

    // devices
    acpi_init(g_ebda_addr);
    pci_init(false);
    rsdt_parse();
    scheduler_init();
    ide_device_t *ideDevices = init_ide();

    init_keyboard();

    printf("Kernel Booted in %ims\n", ticks);
    ext2_read_drive(0);
    consoleMarkInputStart();

    asm volatile("sti");
    for (;;)
        asm volatile("hlt"); // <<<<<<<<<<< CHANGE THE LOOP TO THIS
}

// holy fuck i need to figure out what posix requires that shit is going to melt my fucking brain for sure