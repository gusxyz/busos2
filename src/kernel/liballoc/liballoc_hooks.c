#include "liballoc.h"
#include "../memory.h"
#include "../util.h"

int liballoc_lock()
{
    lockInterrupts();
    return 0;
}

int liballoc_unlock()
{
    unlockInterrupts();
    return 0;
}

void* liballoc_alloc(size_t num_pages)
{
    uint32_t start_vaddr = vmmFindFreePages(num_pages);
    if (start_vaddr == 0)
    {
        return NULL; // Out of virtual address space
    }

    // Step 2: For each virtual page in the block, allocate a physical frame and map it.
    for (size_t i = 0; i < num_pages; i++)
    {
        uint32_t vaddr = start_vaddr + (i * PAGE_SIZE);

        // a. Allocate a physical frame.
        uint32_t paddr = pmmAllocPageFrame();
        if (paddr == 0)
        {
            // CRITICAL: Out of physical memory! We must roll back the allocation.
            // Free all the pages we have already mapped for this request.
            for (size_t j = 0; j < i; j++)
            {
                vmmUnmapPage(start_vaddr + (j * PAGE_SIZE));
            }
            return NULL; // Return failure
        }

        // b. Map the virtual page to the physical frame.
        // Use flags for present and writable for a general-purpose heap.
        vmmMapPage(vaddr, paddr, PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE);
    }
    return (void*) start_vaddr;
}

int liballoc_free(void* ptr, size_t num_pages)
{
    uint32_t start_vaddr = (uint32_t)ptr;

    for (size_t i = 0; i < num_pages; i++)
    {
        uint32_t vaddr = start_vaddr + (i * PAGE_SIZE);
        // memUnmapPage should handle both unmapping the page and freeing the
        // underlying physical frame.
        vmmUnmapPage(vaddr);
    }

    return 0;
}