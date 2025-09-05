#pragma once
#include <stdint.h>

#define COLOR8_BLACK 0
#define COLOR8_LIGHT_GREY 7
#define COLOR8_WHITE 15

#define width 80
#define height 25

extern char *vgaTitle;
void print(const char *s);
void scrollUp();
void newLine();
void reset();
void enableCursor(uint8_t cursor_start, uint8_t cursor_end);
void disableCursor();
void update_cursor(int x, int y);
