#ifndef KEYBOARD_H
#define KEYBOARD_H
#include <util.h>

void init_keyboard();
void keyboardHandler(struct InterruptRegisters *reg);
#endif