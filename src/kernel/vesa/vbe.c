#include "vbe.h"
#include "../memory.h"
#include "psf.h"
#include "../liballoc/liballoc.h"

#define USHRT_MAX 65535 // mlibc will cover this
#define PIXEL uint32_t
extern char _binary_src_kernel_fonts_zap_light18_psf_start;
extern char _binary_src_kernel_fonts_zap_light18_psf_end;
uint16_t *unicode;

void vbeInit(struct multiboot_info *bootInfo)
{
    struct multiboot_info *virtualBootInfo = (struct multiboot_info *)((uint32_t)bootInfo + KERNEL_START);
    if (!virtualBootInfo->flags & (1 << 12))
        return;
    // 1. Get the physical address and dimensions from the Multiboot struct.
    uint32_t framebufferPhysAddr = (uint32_t)virtualBootInfo->framebuffer_addr;
    uint32_t w = virtualBootInfo->framebuffer_width;
    uint32_t h = virtualBootInfo->framebuffer_height;
    uint32_t bpp = virtualBootInfo->framebuffer_bpp;
    scanline = virtualBootInfo->framebuffer_pitch;

    // 2. Calculate the total size of the framebuffer in bytes.
    uint32_t bytesPerPixel = bpp / 8;
    uint32_t totalSizeBytes = w * h * bytesPerPixel;

    // 3. Calculate how many 4KB pages are needed to cover this size.
    // We use CEIL_DIV to round up to the nearest whole page.
    size_t numPages = CEIL_DIV(totalSizeBytes, PAGE_SIZE);

    vmmMapRegion(FRAMEBUFFER_VIRTUAL_ADDR,
                 framebufferPhysAddr,
                 numPages,
                 PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE);
    psfInit();
}

void psfInit()
{
    uint16_t glyph = 0;
    /* cast the address to PSF header struct */
    PSF_font *font = (PSF_font *)&_binary_src_kernel_fonts_zap_light18_psf_start;
    /* is there a unicode table? */
    if (font->flags == 0)
    {
        unicode = NULL;
        return;
    }

    /* get the offset of the table */
    char *s = (char *)((uint8_t *)&_binary_src_kernel_fonts_zap_light18_psf_start +
                       font->headersize +
                       font->numglyph * font->bytesperglyph);
    /* allocate memory for translation table */
    unicode = calloc(USHRT_MAX, 2);
    while (s < (uint8_t *)&_binary_src_kernel_fonts_zap_light18_psf_end)
    {
        uint16_t uc = (uint16_t)((uint8_t *)s[0]);
        if (uc == 0xFF)
        {
            glyph++;
            s++;
            continue;
        }
        else if (uc & 128)
        {
            /* UTF-8 to unicode */
            if ((uc & 32) == 0)
            {
                uc = ((s[0] & 0x1F) << 6) + (s[1] & 0x3F);
                s++;
            }
            else if ((uc & 16) == 0)
            {
                uc = ((((s[0] & 0xF) << 6) + (s[1] & 0x3F)) << 6) + (s[2] & 0x3F);
                s += 2;
            }
            else if ((uc & 8) == 0)
            {
                uc = ((((((s[0] & 0x7) << 6) + (s[1] & 0x3F)) << 6) + (s[2] & 0x3F)) << 6) + (s[3] & 0x3F);
                s += 3;
            }
            else
                uc = 0;
        }
        /* save translation */
        unicode[uc] = glyph;
        s++;
    }
}

void putchar(uint16_t c, int cx, int cy, uint32_t fg, uint32_t bg)
{
    /* cast the address to PSF header struct */
    PSF_font *font = (PSF_font *)&_binary_src_kernel_fonts_zap_light18_psf_start;
    /* we need to know how many bytes encode one row */
    int bytesperline = (font->width + 7) / 8;
    /* unicode translation */
    if (unicode != NULL)
    {
        c = unicode[c];
    }
    /* get the glyph for the character. If there's no
       glyph for a given character, we'll display the first glyph. */
    uint8_t *glyph =
        (uint8_t *)&_binary_src_kernel_fonts_zap_light18_psf_start +
        font->headersize +
        (c > 0 && c < font->numglyph ? c : 0) * font->bytesperglyph;
    /* calculate the upper left corner on screen where we want to display.
       we only do this once, and adjust the offset later. This is faster. */
    int offs =
        (cy * font->height * scanline) +
        (cx * (font->width + 1) * sizeof(PIXEL));
    /* finally display pixels according to the bitmap */
    int x, y, line, mask;
    for (y = 0; y < font->height; y++)
    {
        /* save the starting position of the line */
        line = offs;
        mask = 1 << (font->width - 1);
        /* display a row */
        for (x = 0; x < font->width; x++)
        {
            *((PIXEL *)(FRAMEBUFFER_VIRTUAL_ADDR + line)) = *((unsigned int *)glyph) & mask ? fg : bg;
            /* adjust to the next pixel */
            mask >>= 1;
            line += sizeof(PIXEL);
        }
        /* adjust to the next line */
        glyph += bytesperline;
        offs += scanline;
    }
}
