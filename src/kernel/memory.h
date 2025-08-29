#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

extern uint32_t initial_page_dir[1024];
extern int mem_num_vpages;

#define KERNEL_START 0xC0000000
#define PAGE_SIZE 4096
#define HEAP_START 0XD0000000
#define REC_PAGEDIR ((uint32_t*)0xFFFFF000)
#define REC_PAGETABLE(i) ((uint32_t*) (0xFFC00000 + ((i) << 12)))

#define PAGE_FLAG_PRESENT (1 << 0)
#define PAGE_FLAG_WRITE (1 << 1)
#define PAGE_FLAG_OWNER (1 << 9)

void invalid(uint32_t virtualAddr);
void initMemory(uint32_t memHigh, uint32_t physicalAllocStart);
uint32_t pmmAllocPageFrame();
uint32_t vmmFindFreePages(size_t numPages);
void vmmUnmapPage(uint32_t virtualAddr);
void vmmMapPage(uint32_t virutalAddr, uint32_t physAddr, uint32_t flags);
void syncPageDirs();
void memChangePageDir(uint32_t* pd);
uint32_t* memGetCurrentPageDir();
void pmmFreePageFrame(uint32_t paddr);
void vmmUnmapPage(uint32_t virtualAddr);
bool memIsPagePresent(uint32_t virtualAddr);