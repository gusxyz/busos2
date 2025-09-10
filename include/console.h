#ifndef CONSOLE_H
#define CONSOLE_H
#include <stdint.h>

void console_init();
void consoleMarkInputStart();
void createPrompt();
void clearConsole();
void consolePutC(char c);
#endif