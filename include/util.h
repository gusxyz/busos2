#pragma once
#ifndef UTIL_H
#define UTIL_H
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define is_bit_set(byte_array, bit) (byte_array[(bit) / 8] & (1 << ((bit) % 8)))
#define set_bit(byte_array, bit) (byte_array[(bit) / 8] |= (1 << ((bit) % 8)))
#define CEIL_DIV(a, b) (((a + b) - 1) / b)

struct InterruptRegisters
{
    uint32_t cr2;
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, csm, eflags, useresp, ss;
};

#ifdef __cplusplus
extern "C"
{
#endif
    void serial_init();
    int serialTransmitEmpty();
    void serial_putc(char c);
    void serial_puts(const char *str);
    void serial_putsf(const char *fmt, ...);
    int *serial_putsfn(int *argp, int length, bool sign, int radix);

    // lock interrupts
    static inline void lockInterrupts()
    {
        __asm__ volatile("cli");
    };

    // unlock interrupts
    static inline void unlockInterrupts()
    {
        __asm__ volatile("sti");
    };

    // output port b
    static inline void outb(uint16_t port, uint8_t val)
    {
        __asm__ volatile("outb %b0, %w1" : : "a"(val), "Nd"(port) : "memory");
    };

    // output port l
    static inline void outl(uint16_t port, uint32_t val)
    {
        __asm__ volatile("outl %0, %1" : : "a"(val), "d"(port) : "memory");
    };

    // input on port b
    static inline uint8_t inb(uint16_t port)
    {
        uint8_t ret;
        __asm__ volatile("inb %w1, %b0"
                         : "=a"(ret)
                         : "Nd"(port)
                         : "memory");
        return ret;
    };

    static inline void insl_rep(uint16_t port, void *addr, int count)
    {
        __asm__ volatile(
            "pushw %%es\n\t"          // Save the original ES register
            "movw %%ds, %%ax\n\t"     // Load the current Data Segment into AX
            "movw %%ax, %%es\n\t"     // Set the Extra Segment to the Data Segment
            "rep insl\n\t"            // Execute the repeated string read
            "popw %%es"               // Restore the original ES register
            : "+D"(addr), "+c"(count) // Input/Output: EDI (addr) and ECX (count)
            : "d"(port)               // Input: EDX (port)
            : "eax", "memory"         // Clobbers: EAX is used, and memory is written to
        );
    };

    static inline uint32_t inl(uint16_t port)
    {
        uint32_t ret;
        __asm__ volatile("inl %1, %0"
                         : "=a"(ret)
                         : "Nd"(port)
                         : "memory");
        return ret;
    };

#ifdef __cplusplus
}
#endif
#endif