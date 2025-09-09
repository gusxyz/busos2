#ifndef CONSOLE_H
#define CONSOLE_H
#include <stdint.h>

void consoleInit();
void consoleMarkInputStart();
void createPrompt();
void clearConsole();
void consolePutC(char c);
#endif