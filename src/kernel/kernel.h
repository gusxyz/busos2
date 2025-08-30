#pragma once
#include "vga.h"
#include "gdt/gdt.h"
#include "idt/idt.h"
#include "timer.h"
#include "multiboot.h"
#include "memory.h"
#include "keyboard.h"
#include "liballoc/liballoc.h"
#include "pci.h"
#include "ide.h"

void kernel_main(uint32_t magic, struct multiboot_info* bootInfo);
