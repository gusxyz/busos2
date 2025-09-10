#include <memory.h>
#include <multiboot.h>
#include <util.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#define CEIL_DIV(a, b) (((a + b) - 1) / b)

static uint32_t pageFrameMin;
static uint32_t pageFrameMax;
static uint32_t totalAlloc;
int mem_num_vpages;

#define BYTE 8
#define NUM_PAGES_DIRS 256
#define NUM_PAGE_FRAMES (0x10000000 / 0x1000 / BYTE)
static uint32_t next_free_vaddr = HEAP_START;

uint8_t physicalMemoryBitmap[NUM_PAGE_FRAMES / BYTE];
static uint32_t pageDirs[NUM_PAGES_DIRS][1024] __attribute__((aligned(PAGE_SIZE)));
static uint8_t pageDirUsed[NUM_PAGES_DIRS];

void pmmInit(uint32_t memLow, uint32_t memHigh)
{
    pageFrameMin = CEIL_DIV(memLow, 0x1000);
    pageFrameMax = memHigh / 0x1000;
    totalAlloc = 0;
    serial_putsf("--- PMM Initialization ---\n");
    serial_putsf("memLow (physicalAllocStart): 0x%x\n", memLow);
    serial_putsf("memHigh (mem_upper * 1024): 0x%x\n", memHigh);
    serial_putsf("Calculated pageFrameMin: %d (0x%x)\n", pageFrameMin, pageFrameMin);
    serial_putsf("Calculated pageFrameMax: %d (0x%x)\n", pageFrameMax, pageFrameMax);
    serial_putsf("Search will start at byte_index: %d\n", pageFrameMin / 8);
    serial_putsf("Search will end before byte_index: %d\n", pageFrameMax / 8);
    serial_putsf("--------------------------\n");
    memset(physicalMemoryBitmap, 0, sizeof(physicalMemoryBitmap));
}

uint32_t vmmFindFreePages(size_t numPages)
{
    uint32_t consecutive_pages = 0;
    uint32_t start_addr = 0;
    uint32_t current_addr = next_free_vaddr;

    while (current_addr >= HEAP_START)
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
        if (current_addr < next_free_vaddr)
        {
            return 0; // Out of virtual memory
        }
    }

    return 0; // Could not find a suitable block before kernel space
}

void init_memory(uint32_t memHigh, uint32_t physicalAllocStart)
{
    initial_page_dir[0] = 0;
    invalid(0);
    initial_page_dir[1023] = ((uint32_t)initial_page_dir - KERNEL_START) | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE;
    invalid(0xFFFFF000);

    serial_putsf("Identity mapping first 1MB...\n");
    vmmMapRegion(0x0, 0x0, 256, PAGE_FLAG_WRITE);

    pmmInit(physicalAllocStart, memHigh);
    memset(pageDirs, 0, 0x1000 * NUM_PAGES_DIRS);
    memset(pageDirUsed, 0, NUM_PAGES_DIRS);
}

void invalid(uint32_t virtualAddr)
{
    asm volatile("invlpg %0" ::"m"(virtualAddr));
}

uint32_t pmmAllocPageFrame()
{
    for (uint32_t b = pageFrameMin / 8; b < pageFrameMax / 8; b++)
    {
        uint8_t byte = physicalMemoryBitmap[b];
        if (byte == 0xFF)
        {
            continue;
        }

        for (uint32_t i = 0; i < 8; i++)
        {
            bool used = byte >> i & 1;

            if (!used)
            {
                byte ^= (-1 ^ byte) & (1 << i);
                physicalMemoryBitmap[b] = byte;
                totalAlloc++;

                uint32_t frameNumber = (b * 8) + i;
                return frameNumber * PAGE_SIZE;
            }
        }
    }

    serial_putsf("PMM: Out of physical memory!\n");
    return 0;
}

void pmmFreePageFrame(uint32_t paddr)
{
    uint32_t frameNum = paddr / PAGE_SIZE;

    uint32_t byteIndex = frameNum / BYTE;
    uint32_t bitIndex = frameNum % BYTE;

    if ((physicalMemoryBitmap[byteIndex] & (1 << bitIndex)) == 0)
    {

        return;
    }

    // Clear the bit to mark it as free.
    //  We do this by ANDing with the inverse of the bit we want to clear.
    physicalMemoryBitmap[byteIndex] &= ~(1 << bitIndex);

    totalAlloc--;
}

void vmmUnmapPage(uint32_t virtualAddr)
{
    uint32_t pdIndex = virtualAddr >> 22;
    uint32_t ptIndex = virtualAddr >> 12 & 0x3FF;

    uint32_t *prevPageDir = 0;

    if (virtualAddr >= KERNEL_START)
    {
        prevPageDir = memGetCurrentPageDir();
        if (prevPageDir != initial_page_dir)
        {
            memChangePageDir(initial_page_dir);
        }
    }

    uint32_t *pageDir = REC_PAGEDIR;

    if (pageDir[pdIndex] & PAGE_FLAG_PRESENT)
    {
        uint32_t *pt = REC_PAGETABLE(pdIndex);

        if (pt[ptIndex] & PAGE_FLAG_PRESENT)
        {

            uint32_t paddr = pt[ptIndex] & ~0xFFF;

            if (paddr != 0)
            {
                pmmFreePageFrame(paddr);
            }

            //
            pt[ptIndex] = 0;
            mem_num_vpages--;

            invalid(virtualAddr);
        }
    }

    if (prevPageDir != 0)
    {
        if (prevPageDir != initial_page_dir)
        {
            memChangePageDir(prevPageDir);
        }
    }
}

uint32_t *memGetCurrentPageDir()
{
    uint32_t pd;
    asm volatile("mov %%cr3, %0" : "=r"(pd));
    pd += KERNEL_START;

    return (uint32_t *)pd;
}

void memChangePageDir(uint32_t *pd)
{
    pd = (uint32_t *)(((uint32_t)pd) - KERNEL_START);
    asm volatile("mov %0, %%eax \n mov %%eax, %%cr3 \n" ::"m"(pd));
}

void syncPageDirs()
{
    for (int i = 0; i < NUM_PAGES_DIRS; i++)
    {
        if (pageDirUsed[i])
        {
            uint32_t *pageDir = pageDirs[i];

            for (int i = 768; i < 1023; i++)
            {
                pageDir[i] = initial_page_dir[i] & ~PAGE_FLAG_OWNER;
            }
        }
    }
}

bool memIsPagePresent(uint32_t virtualAddr)
{

    uint32_t pdIndex = virtualAddr >> 22;

    uint32_t ptIndex = (virtualAddr >> 12) & 0x3FF;

    uint32_t *pageDir = REC_PAGEDIR;
    uint32_t pde = pageDir[pdIndex];

    if (!(pde & PAGE_FLAG_PRESENT))
    {
        return false;
    }

    uint32_t *pageTable = REC_PAGETABLE(pdIndex);
    uint32_t pte = pageTable[ptIndex];

    // Check if the page is present.
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
        if (prevPageDir != initial_page_dir)
        {
            memChangePageDir(initial_page_dir);
        }
    }

    uint32_t pdIndex = virutalAddr >> 22;
    uint32_t ptIndex = virutalAddr >> 12 & 0x3FF;

    uint32_t *pageDir = REC_PAGEDIR;
    uint32_t *pt = REC_PAGETABLE(pdIndex);

    if (!(pageDir[pdIndex] & PAGE_FLAG_PRESENT))
    {
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

    if (prevPageDir != 0)
    {
        syncPageDirs();

        if (prevPageDir != initial_page_dir)
        {
            memChangePageDir(prevPageDir);
        }
    }
}

void vmmMapRegion(uint32_t virtualAddr, uint32_t physAddr, size_t numPages, uint32_t flags)
{
    for (size_t i = 0; i < numPages; i++)
    {
        uint32_t v = virtualAddr + (i * PAGE_SIZE);
        uint32_t p = physAddr + (i * PAGE_SIZE);
        vmmMapPage(v, p, flags);
    }
}

void vmmUnmapRegion(uint32_t virtualAddr, size_t numPages)
{
    for (size_t i = 0; i < numPages; i++)
    {
        uint32_t v = virtualAddr + (i * PAGE_SIZE);
        // We can just call pmmFreePageFrame directly since we dont care about the other stuff
        vmmUnmapPage(v);
    }
}

void *vmmAlloc(uint32_t phys_addr, size_t num_pages, uint32_t flags)
{
    uint32_t virt_addr = vmmFindFreePages(num_pages);
    if (virt_addr == 0)
    {
        printf("vmmAlloc: out of virtual memory!\n");
        return NULL;
    }

    vmmMapRegion(virt_addr, phys_addr, num_pages, flags);

    return (void *)virt_addr;
}