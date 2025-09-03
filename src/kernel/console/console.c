#include "console.h"
#include "../vesa/vbe.h"
#include "../util.h"

static int console_x = 0;
static int console_y = 0;

#define FONT_WIDTH 8
#define FONT_HEIGHT 18

static uint32_t screen_width;
static uint32_t screen_height;

void console_putc(char c)
{
    // Assuming font dimensions are available
    unsigned int term_width = screen_width / FONT_WIDTH;
    unsigned int term_height = screen_height / FONT_HEIGHT;

    if (c == '\n')
    {
        console_x = 0;
        console_y++;
    }
    else if (c == '\r')
    {
        console_x = 0;
    }
    else if (c == '\b')
    {
        if (console_x > 0)
        {
            console_x--;
            // Optional: print a space to erase the character
            putchar(' ', console_x, console_y, 0x000000, 0x000000);
        }
    }
    else
    {
        // Your existing putchar from vbe.c
        putchar(c, console_x, console_y, 0xFFFFFF, 0x000000); // White on black
        console_x++;
        if (console_x >= term_width)
        {
            console_x = 0;
            console_y++;
        }
    }

    // Handle scrolling if needed
    if (console_y >= term_height)
        scroll_screen();
}

void scroll_screen()
{
    // Calculate the size of a character row in bytes
    unsigned int row_size = scanline * FONT_HEIGHT;

    // Move every row up by one
    for (unsigned int y = 0; y < screen_height - FONT_HEIGHT; y++)
        memcpy((void *)(FRAMEBUFFER_VIRTUAL_ADDR + y * scanline),
               (void *)(FRAMEBUFFER_VIRTUAL_ADDR + (y + FONT_HEIGHT) * scanline),
               scanline);

    // Clear the last line
    memset((void *)(FRAMEBUFFER_VIRTUAL_ADDR + (screen_height - FONT_HEIGHT) * scanline),
           0, row_size);
}