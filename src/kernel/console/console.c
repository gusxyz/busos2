#define SSFN_CONSOLEBITMAP_TRUECOLOR
#include "console.h"
#include "../vesa/vbe.h"
#include "../util.h"
#include <ssfn.h>

extern uint32_t vbe_width;
extern uint32_t vbe_height;
extern ssfn_font_t _binary_src_kernel_fonts_font_sfn_start;
ssfn_font_t *ssfn_src;
ssfn_buf_t ssfn_dst;

// Variables to track the start of the user's editable input line
static uint32_t input_start_x = 0;
static uint32_t input_start_y = 0;
#define MONOSPACE ssfn_src->width / 1.5

// Initialize console variables
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

// CORRECTED: Prints the prompt first, then marks the editable starting point.
void consoleMarkInputStart()
{
    createPrompt();

    input_start_x = ssfn_dst.x;
    input_start_y = ssfn_dst.y;
}

void createPrompt()
{
    ssfn_putc('>');
    ssfn_dst.x = ssfn_src->width; // Position cursor after the '>'
    ssfn_putc(' ');
    ssfn_dst.x = (ssfn_src->width * 2) / 1.5; // Position cursor after the ' '
}

// CORRECTED: Erases a block exactly the size of the font's max dimensions.
void clear_char_at_cursor()
{
    uint32_t *fb_ptr = (uint32_t *)ssfn_dst.ptr;
    uint32_t pitch_in_pixels = ssfn_dst.p / 4;

    // Loop from current y to y + height (no +1)
    for (unsigned int row = ssfn_dst.y; row < ssfn_dst.y + ssfn_src->height; row++)
    {
        // Loop from current x to x + width (no +1)
        for (unsigned int col = ssfn_dst.x; col < ssfn_dst.x + ssfn_src->width; col++)
        {
            if (col < vbe_width && row < vbe_height)
            {
                fb_ptr[row * pitch_in_pixels + col] = ssfn_dst.bg;
            }
        }
    }
}

void consolePutC(char c)
{
    if (c == '\n')
    {
        ssfn_dst.x = 0;
        ssfn_dst.y += ssfn_src->height;
        serial_putc('\n');

        // Let the main kernel loop decide when to reprint the prompt.
        // This makes the console logic cleaner.
    }
    else if (c == '\r')
    {
        ssfn_dst.x = 0;
    }
    else if (c == '\b')
    {
        // Check if the cursor is within the editable area
        if (ssfn_dst.y > input_start_y || (ssfn_dst.y == input_start_y && ssfn_dst.x > input_start_x))
        {
            // Handle wrapping to the previous line
            if (ssfn_dst.x == 0)
            {
                ssfn_dst.y -= ssfn_src->height;
                // Go to the last character grid position on the line
                ssfn_dst.x = vbe_width - (vbe_width % MONOSPACE) - MONOSPACE;
            }
            else
            {
                // Just move back one character width
                ssfn_dst.x -= MONOSPACE;
            }

            clear_char_at_cursor();

            serial_putc('\b');
            serial_putc(' ');
            serial_putc('\b');
        }
    }
    else
    {
        // --- CRITICAL FIX: FORCING MONOSPACED BEHAVIOR ---

        // 1. Check for line wrapping before drawing.
        if (ssfn_dst.x + ssfn_src->width > vbe_width)
        {
            ssfn_dst.x = 0;
            ssfn_dst.y += ssfn_src->height;
        }

        // 2. Store the current grid position.
        uint32_t current_x = ssfn_dst.x;

        // 3. Render the character. This advances ssfn_dst.x by a variable amount.
        ssfn_putc(c);
        serial_putc(c);

        // 4. Force the cursor to the next fixed-width grid position.
        //    This makes printing and backspacing symmetrical.
        ssfn_dst.x = current_x + MONOSPACE;
    }

    // Optional: Keep this for debugging if you want.
    // serial_putsf("x=%i ", ssfn_dst.x);
}