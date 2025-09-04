#pragma once
#include <liballoc.h>
#include "gdt/gdt.h"
#include "idt/idt.h"
#include "timer.h"
#include "multiboot.h"
#include "memory.h"
#include "keyboard.h"
#include "drivers/pci.h"
#include "drivers/ide.h"
#include "filesystem/filesystem.h"
#include "vesa/vbe.h"
#include "console/console.h"
#include "rsdp.h"

void kernel_main(uint32_t magic, multiboot_info_t *bootInfo);