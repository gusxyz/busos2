#define SSFN_CONSOLEBITMAP_TRUECOLOR
#include <console.h>
#include <vbe.h>
#include <util.h>
#include <ssfn.h>
#include <spinlock.h>

extern uint32_t vbe_width;
extern uint32_t vbe_height;
extern ssfn_font_t _binary_src_kernel_fonts_font_sfn_start;
ssfn_font_t *ssfn_src;
ssfn_buf_t ssfn_dst;

static uint32_t input_start_x = 0;
static uint32_t input_start_y = 0;
#define MONOSPACE ssfn_src->width / 1.5
extern spinlock_t console_lock = false;

void consoleInit()
{
    serial_putsf("Initializing Console...\n");
    ssfn_src = &_binary_src_kernel_fonts_font_sfn_start;
    ssfn_dst.ptr = (uint8_t *)FRAMEBUFFER_VIRTUAL_ADDR;
    ssfn_dst.p = scanline;
    ssfn_dst.fg = 0xFFFFFFFF;
    ssfn_dst.bg = 0;
    ssfn_dst.x = 0;
    ssfn_dst.y = 0;
}

void clearConsole()
{
    memset(ssfn_dst.ptr, 0, vbe_height * ssfn_dst.p);
}

void consoleMarkInputStart()
{
    createPrompt();
    input_start_x = ssfn_dst.x;
    input_start_y = ssfn_dst.y;
}

void createPrompt()
{
    ssfn_putc('>');
    ssfn_dst.x = ssfn_src->width;
    ssfn_putc(' ');
    ssfn_dst.x = (ssfn_src->width * 2) / 1.5;
}

void clearCharAtCursor()
{
    uint32_t *fb_ptr = (uint32_t *)ssfn_dst.ptr;
    uint32_t pitch_in_pixels = ssfn_dst.p / 4;

    // Calculate the actual area to clear (current cursor position)
    uint32_t clear_x = ssfn_dst.x;
    uint32_t clear_y = ssfn_dst.y;

    // Make sure we don't go out of bounds
    if (clear_x >= vbe_width || clear_y >= vbe_height)
    {
        return;
    }

    // Calculate the actual width to clear (minimum of font width or remaining space)
    uint32_t clear_width = ssfn_src->width;
    if (clear_x + clear_width > vbe_width)
    {
        clear_width = vbe_width - clear_x;
    }

    // Calculate the actual height to clear
    uint32_t clear_height = ssfn_src->height;
    if (clear_y + clear_height > vbe_height)
    {
        clear_height = vbe_height - clear_y;
    }

    // Clear the character area
    for (uint32_t row = 0; row < clear_height; row++)
    {
        for (uint32_t col = 0; col < clear_width; col++)
        {
            uint32_t pixel_x = clear_x + col;
            uint32_t pixel_y = clear_y + row;

            if (pixel_x < vbe_width && pixel_y < vbe_height)
            {
                fb_ptr[pixel_y * pitch_in_pixels + pixel_x] = ssfn_dst.bg;
            }
        }
    }
}
void consoleScroll()
{
    uint32_t font_height = ssfn_src->height;
    uint32_t pitch = ssfn_dst.p;
    uint8_t *dest = ssfn_dst.ptr;
    uint8_t *src = ssfn_dst.ptr + (pitch * font_height);

    // Calculate the size to move (everything except the top line)
    size_t move_size = pitch * (vbe_height - font_height);

    memmove(dest, src, move_size);

    // Clear the last line (now empty after scrolling)
    uint8_t *last_line_start = ssfn_dst.ptr + (pitch * (vbe_height - font_height));
    memset(last_line_start, 0, pitch * font_height);
}

void isCursorVisible()
{
    // Handle horizontal wrapping
    if (ssfn_dst.x >= vbe_width)
    {
        ssfn_dst.x = 0;
        ssfn_dst.y += ssfn_src->height;
    }

    // Handle vertical scrolling
    if (ssfn_dst.y + ssfn_src->height > vbe_height)
    {
        consoleScroll();
        // After scrolling, adjust all y positions
        ssfn_dst.y -= ssfn_src->height;
        input_start_y -= ssfn_src->height;
    }
}

void consolePutC(char c)
{
    if (c == '\n')
    {
        ssfn_dst.x = 0;
        ssfn_dst.y += ssfn_src->height;
        isCursorVisible(); // Check if we need to scroll after newline
    }
    else if (c == '\r')
    {
        ssfn_dst.x = 0;
    }
    else if (c == '\b')
    {
        if (ssfn_dst.y > input_start_y || (ssfn_dst.y == input_start_y && ssfn_dst.x > input_start_x))
        {
            ssfn_dst.x -= ssfn_src->width;

            if (ssfn_dst.x < 0 && ssfn_dst.y > input_start_y)
            {
                ssfn_dst.y -= ssfn_src->height;
                ssfn_dst.x = vbe_width - ssfn_src->width;
            }

            clearCharAtCursor();
            serial_putc('\b');
        }
    }
    else
    {
        // Check if we need to scroll BEFORE drawing
        isCursorVisible();

        ssfn_putc(c);
        serial_putc(c);

        // Check if we need to scroll AFTER drawing
        isCursorVisible();
    }
}
