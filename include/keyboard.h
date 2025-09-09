#ifndef KEYBOARD_H
#define KEYBOARD_H
#include <util.h>

void initKeyboard();
void keyboardHandler(struct InterruptRegisters *reg);
#endif