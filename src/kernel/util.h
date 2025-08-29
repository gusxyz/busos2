#pragma once
#include <stdint.h>

void *memset(void *dest, char val, uint32_t count);

// lock interrupts
static inline int lockInterrupts()
{
    __asm__ volatile ("cli");
}

// unlock interrupts
static inline int unlockInterrupts()
{
    __asm__ volatile ("sti");
}

// output port b
static inline void outb(uint16_t port, uint8_t val)
{
    __asm__ volatile ( "outb %b0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

#define CEIL_DIV(a,b) (((a + b) - 1) / b)

// input on port b
static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile ( "inb %w1, %b0"
                   : "=a"(ret)
                   : "Nd"(port)
                   : "memory");
    return ret;
}

extern struct InterruptRegisters
{
    uint32_t cr2;
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, csm, eflags, useresp, ss;
};