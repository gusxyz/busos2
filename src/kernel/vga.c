#include "vga.h"
#include "util.h"

uint16_t column = 0;
uint16_t line = 0;
uint16_t *const vgaMemory = (uint16_t *const)0xC00B8000;
const uint16_t defaultColor = (COLOR8_WHITE << 8) | (COLOR8_BLACK << 12);
uint16_t currentColor = defaultColor;

char *vgaTitle =
    "$$$$$$$\\                             $$$$$$\\   $$$$$$\\\n"
    "$$  __$$\\                           $$  __$$\\ $$  __$$\\\n"
    "$$ |  $$ |$$\\   $$\\  $$$$$$$\\       $$ /  $$ |$$ /  \\__|\n"
    "$$$$$$$\\ |$$ |  $$ |$$  _____|      $$ |  $$ |\\$$$$$$\\\n"
    "$$  __$$\\ $$ |  $$ |\\$$$$$$\\        $$ |  $$ | \\____$$\\\n"
    "$$ |  $$ |$$ |  $$ | \\____$$\\       $$ |  $$ |$$\\   $$ |\n"
    "$$$$$$$  |\\$$$$$$  |$$$$$$$  |       $$$$$$  |\\$$$$$$  |\n./"
    "\\_______/  \\______/ \\_______/        \\______/  \\______/";

void reset()
{
    line = 0;
    column = 0;
    currentColor = defaultColor;

    for (uint16_t y = 0; y < height; y++)
    {
        for (uint16_t x = 0; x < width; x++)
        {
            vgaMemory[y * width + x] = ' ' | defaultColor;
        }
    }
}

void newLine()
{
    if (line < height - 1)
    {
        line++;
        column = 0;
    }
    else
    {
        scrollUp();
        column = 0;
    }
}

void scrollUp()
{
    for (uint16_t y = 0; y < height; y++)
    {
        for (uint16_t x = 0; x < width; x++)
        {
            vgaMemory[(y - 1) * width + x] = vgaMemory[y * width + x];
        }
    }

    for (uint16_t x = 0; x < width; x++)
    {
        vgaMemory[(height - 1) * width + x] = ' ' | currentColor;
    }
}
void enableCursor(uint8_t cursor_start, uint8_t cursor_end)
{
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | cursor_start);

    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | cursor_end);
}

void disableCursor()
{
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}

void update_cursor(int x, int y)
{
    uint16_t pos = y * width + x;

    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void print(const char *string)
{
    while (*string)
    {
        switch (*string)
        {
        case '\n':
            newLine();
            break;
        case '\b':
            if (column == 0 && line != 0)
            {
                line--;
                column = width;
            }

            vgaMemory[line * width + (column--)] = ' ' | currentColor;
            break;
        case '\r':
            column = 0;
            break;
        case '\t':
            if (column == width)
                newLine();

            uint16_t tabLen = 4 - (column % 4);
            while (tabLen != 0)
            {
                vgaMemory[line * width + (column++)] = ' ' | currentColor;
                tabLen--;
            }
            break;
        default:
            if (column == width)
                newLine();

            vgaMemory[line * width + (column++)] = *string | currentColor;
            break;
        }
        string++;
    }

    update_cursor(column, line + 1);
}
