#include <vbe.h>
#include <memory.h>
#include <util.h>

uint32_t scanline;
uint32_t vbe_width;
uint32_t vbe_height;

void vbe_init(multiboot_info_t *bootInfo)
{
    multiboot_info_t *virtualBootInfo = (multiboot_info_t *)((uint32_t)bootInfo + KERNEL_START);
    if (!virtualBootInfo->flags & (1 << 12))
        return;
    // 1. Get the physical address and dimensions from the Multiboot struct.
    uint32_t framebufferPhysAddr = (uint32_t)virtualBootInfo->framebuffer_addr;
    serial_putsf("Initializing VBE. Framebuffer found: 0x%x\n", framebufferPhysAddr);
    vbe_width = virtualBootInfo->framebuffer_width;
    vbe_height = virtualBootInfo->framebuffer_height;
    uint32_t bpp = virtualBootInfo->framebuffer_bpp;
    scanline = virtualBootInfo->framebuffer_pitch;

    // 2. Calculate the total size of the framebuffer in bytes.
    uint32_t bytesPerPixel = bpp / 8;
    uint32_t totalSizeBytes = vbe_width * vbe_height * bytesPerPixel;

    // 3. Calculate how many 4KB pages are needed to cover this size.
    // We use CEIL_DIV to round up to the nearest whole page.
    size_t numPages = CEIL_DIV(totalSizeBytes, PAGE_SIZE);

    vmmMapRegion(FRAMEBUFFER_VIRTUAL_ADDR,
                 framebufferPhysAddr,
                 numPages,
                 PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE);

    serial_putsf("Mapped Framebuffer To: 0x%X\n", FRAMEBUFFER_VIRTUAL_ADDR);
}
