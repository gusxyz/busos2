#pragma once
#include "util.h"

void initTimer();
void onIrq0(struct InterruptRegisters *reg);
void sleep(uint32_t millis);