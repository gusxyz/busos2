#ifndef TIMER_H
#define TIMER_H
#pragma once

#include <util.h>
#ifdef __cplusplus
extern "C"
{
#endif
    void initTimer();
    void onIrq0(struct InterruptRegisters *reg);
    void sleep(uint32_t millis);
    extern uint64_t ticks;
    extern uint64_t g_Quantum;
#ifdef __cplusplus
}
#endif
#endif