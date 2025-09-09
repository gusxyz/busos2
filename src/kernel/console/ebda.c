#include <ebda.h>

uint32_t getEBDA(multiboot_info_t *bootInfo)
{
    if (bootInfo->flags & MULTIBOOT_INFO_MEM_MAP)
    {
        multiboot_mmap_entry_t *mmap = (multiboot_mmap_entry_t *)bootInfo->mmap_addr;

        while ((uint32_t)mmap < bootInfo->mmap_addr + bootInfo->mmap_length)
        {
            if (mmap->type == MULTIBOOT_MEMORY_RESERVED && mmap->addr_high == 0 && mmap->addr_low < 0x100000)
            {
                return mmap->addr_low;
            }
            mmap = (multiboot_mmap_entry_t *)((uint32_t)mmap + mmap->size + sizeof(mmap->size));
        }
    }
}