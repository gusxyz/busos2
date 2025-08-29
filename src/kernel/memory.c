#include "memory.h"
#include "multiboot.h"
#include "util.h"
#include <stdbool.h>
#include <stddef.h>

static uint32_t pageFrameMin;
static uint32_t pageFrameMax;
static uint32_t totalAlloc;
int mem_num_vpages;

#define BYTE 8
#define NUM_PAGES_DIRS 256
#define NUM_PAGE_FRAMES (0x10000000 / 0x1000 / BYTE)
static uint32_t next_free_vaddr = HEAP_START;
#define HEAP_START 0x10000000

uint8_t physicalMemoryBitmap[NUM_PAGE_FRAMES / BYTE];
static uint32_t pageDirs[NUM_PAGES_DIRS][1024] __attribute__((aligned(PAGE_SIZE)));
static uint8_t pageDirUsed[NUM_PAGES_DIRS];

void pmmInit(uint32_t memLow, uint32_t memHigh)
{
    pageFrameMin = CEIL_DIV(memLow, 0x1000);
    pageFrameMax = memHigh / 0x1000;
    totalAlloc = 0;

    memset(physicalMemoryBitmap, 0, sizeof(physicalMemoryBitmap));
}

uint32_t vmmFindFreePages(size_t numPages)
{
    uint32_t consecutive_pages = 0;
    uint32_t start_addr = 0;
    uint32_t current_addr = next_free_vaddr;

    while (current_addr < KERNEL_START)
    {
        if (!memIsPagePresent(current_addr))
        {
            if (consecutive_pages == 0)
            {
                start_addr = current_addr;
            }
            consecutive_pages++;
            if (consecutive_pages >= numPages)
            {
                next_free_vaddr = start_addr + (numPages * PAGE_SIZE);
                return start_addr;
            }
        }
        else
        {
            consecutive_pages = 0;
        }
        
        current_addr += PAGE_SIZE;

        // Handle address wrapping or exhaustion
        if (current_addr < next_free_vaddr) {
             return 0; // Out of virtual memory
        }
    }

    return 0; // Could not find a suitable block before kernel space
}

void initMemory(uint32_t memHigh, uint32_t physicalAllocStart)
{
    initial_page_dir[0] = 0;
    invalid(0);
    initial_page_dir[1023] = ((uint32_t) initial_page_dir - KERNEL_START) | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE;
    invalid(0xFFFFF000);

    pmmInit(physicalAllocStart, memHigh);
    memset(pageDirs, 0, 0x1000 * NUM_PAGES_DIRS);
    memset(pageDirUsed, 0, NUM_PAGES_DIRS); 
}

void invalid(uint32_t virtualAddr)
{
    asm volatile("invlpg %0" :: "m"(virtualAddr));
}

uint32_t pmmAllocPageFrame()
{
    uint32_t start = pageFrameMin / 8 + ((pageFrameMin & 7) != 0 ? 1 : 0);
    uint32_t end = pageFrameMax / 8 - ((pageFrameMax & 7) != 0 ? 1 : 0);

    for (uint32_t b = start; b < end; b++){
        uint8_t byte = physicalMemoryBitmap[b];
        if (byte == 0xFF){
            continue;
        }

        for (uint32_t i = 0; i < 8; i++){
            bool used = byte >> i & 1;

            if (!used){
                byte ^= (-1 ^ byte) & (1 << i);
                physicalMemoryBitmap[b] = byte;
                totalAlloc++;

                uint32_t addr = (b*8*i) * 0x1000;
                return addr;
            }
        }
        
    }

    return 0;
}

void pmmFreePageFrame(uint32_t paddr)
{
    // 1. Convert the physical address to a page frame number.
    // A frame number is simply the address divided by the page size.
    uint32_t frameNum = paddr / PAGE_SIZE;

    // 2. Calculate the position of the frame's bit in the bitmap.
    uint32_t byteIndex = frameNum / BYTE;
    uint32_t bitIndex  = frameNum % BYTE;

    // 3. Check if the page was actually allocated.
    if ((physicalMemoryBitmap[byteIndex] & (1 << bitIndex)) == 0 )
    {
        // This is a double free! You might want to handle this error,
        // e.g., by printing a warning or panicking the kernel.
        // For now, we'll just ignore it.
        return; 
    }

    // 4. Clear the bit to mark it as free.
    // We do this by ANDing with the inverse of the bit we want to clear.
    physicalMemoryBitmap[byteIndex] &= ~(1 << bitIndex);

    totalAlloc--;
}

void vmmUnmapPage(uint32_t virtualAddr)
{
    uint32_t pdIndex = virtualAddr >> 22;
    uint32_t ptIndex = virtualAddr >> 12 & 0x3FF;

    uint32_t *prevPageDir = 0;

    // Handle potential access to kernel page directory
    if (virtualAddr >= KERNEL_START) {
        prevPageDir = memGetCurrentPageDir();
        if (prevPageDir != initial_page_dir) {
            memChangePageDir(initial_page_dir);
        }
    }

    uint32_t* pageDir = REC_PAGEDIR;
    
    // Check if the corresponding page table exists
    if (pageDir[pdIndex] & PAGE_FLAG_PRESENT)
    {
        uint32_t* pt = REC_PAGETABLE(pdIndex);

        // Check if the page itself is mapped
        if (pt[ptIndex] & PAGE_FLAG_PRESENT)
        {
            // 1. Get the physical address from the Page Table Entry (PTE).
            // We need to mask out the flags (lower 12 bits) to get the address.
            uint32_t paddr = pt[ptIndex] & ~0xFFF;

            // 2. Free the physical frame associated with this page.
            if (paddr != 0) { // Don't try to free the null page
                pmmFreePageFrame(paddr);
            }

            // 3. Clear the PTE to unmap the page.
            pt[ptIndex] = 0;
            mem_num_vpages--; // Decrement your counter for mapped pages

            // 4. Invalidate the TLB entry for this virtual address.
            invalid(virtualAddr);
        }
    }

    // Restore previous page directory if it was changed
    if (prevPageDir != 0) {
        if (prevPageDir != initial_page_dir) {
            memChangePageDir(prevPageDir);
        }
    }
}

uint32_t* memGetCurrentPageDir()
{
    uint32_t pd;
    asm volatile("mov %%cr3, %0": "=r"(pd));
    pd += KERNEL_START;

    return (uint32_t*) pd;
}

void memChangePageDir(uint32_t* pd)
{
    pd = (uint32_t*) (((uint32_t)pd)-KERNEL_START);
    asm volatile("mov %0, %%eax \n mov %%eax, %%cr3 \n" :: "m"(pd));
}

void syncPageDirs()
{
    for (int i = 0; i < NUM_PAGES_DIRS; i++){
        if (pageDirUsed[i]){
            uint32_t* pageDir = pageDirs[i];

            for (int i = 768; i < 1023; i++){
                pageDir[i] = initial_page_dir[i] & ~PAGE_FLAG_OWNER;
            }
        }
    }
}

bool memIsPagePresent(uint32_t virtualAddr)
{
    // 1. Calculate the indices for the page directory and page table.
    // The top 10 bits (31-22) are the page directory index.
    uint32_t pdIndex = virtualAddr >> 22;

    // The next 10 bits (21-12) are the page table index.
    uint32_t ptIndex = (virtualAddr >> 12) & 0x3FF;

    // 2. Access the Page Directory Entry (PDE).
    uint32_t* pageDir = REC_PAGEDIR;
    uint32_t pde = pageDir[pdIndex];

    // 3. Check if the page table itself is present.
    // If the PDE's present flag is not set, the entire 4MB region
    // covered by this page table is unmapped.
    if (!(pde & PAGE_FLAG_PRESENT))
    {
        return false;
    }

    // 4. If the page table exists, access the Page Table Entry (PTE).
    uint32_t* pageTable = REC_PAGETABLE(pdIndex);
    uint32_t pte = pageTable[ptIndex];

    // 5. Check if the page is present.
    // The present flag in the PTE determines if this specific 4KB page is mapped.
    if (!(pte & PAGE_FLAG_PRESENT))
    {
        return false;
    }

    // If both the PDE and PTE are present, the address is mapped.
    return true;
}

void vmmMapPage(uint32_t virutalAddr, uint32_t physAddr, uint32_t flags)
{
    uint32_t *prevPageDir = 0;

    if (virutalAddr >= KERNEL_START)
    {
        prevPageDir = memGetCurrentPageDir();
        if (prevPageDir != initial_page_dir){
            memChangePageDir(initial_page_dir);
        }
    }

    uint32_t pdIndex = virutalAddr >> 22;
    uint32_t ptIndex = virutalAddr >> 12 & 0x3FF;
    
    uint32_t* pageDir = REC_PAGEDIR;
    uint32_t* pt = REC_PAGETABLE(pdIndex);

    if (!(pageDir[pdIndex] & PAGE_FLAG_PRESENT)){
        uint32_t ptPAddr = pmmAllocPageFrame();
        pageDir[pdIndex] = ptPAddr | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE | PAGE_FLAG_OWNER | flags;
        invalid(virutalAddr);

        for (uint32_t i = 0; i < 1024; i++)
        {
            pt[i] = 0;
        }
    }

    pt[ptIndex] = physAddr | PAGE_FLAG_PRESENT | flags;
    mem_num_vpages++;
    invalid(virutalAddr);

    if (prevPageDir != 0){
        syncPageDirs();

        if (prevPageDir != initial_page_dir){
            memChangePageDir(prevPageDir);
        }
    }
}

