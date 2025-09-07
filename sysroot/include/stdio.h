#pragma once
#ifndef STDIO_H
#define STDIO_H

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C"
{
#endif
    void putc(char c);
    void puts(const char *str);
    void printf(const char *fmt, ...);
    void vprintf(const char *fmt, va_list args);
    void printf_number_va(va_list args, int, bool, int);
    int strcmp(const char *s1, const char *s2); // needs to be moved to string.h or we just need to get newlib working

    extern void x86_div64_32(uint64_t, uint32_t, uint64_t *, uint32_t *);
    extern const char possibleChars[];
#ifdef __cplusplus
}
#endif

#define PRINTF_STATE_START 0
#define PRINTF_STATE_LENGTH 1
#define PRINTF_STATE_SHORT 2
#define PRINTF_STATE_LONG 3
#define PRINTF_STATE_SPEC 4

#define PRINTF_LENGTH_START 0
#define PRINTF_LENGTH_SHORT_SHORT 1
#define PRINTF_LENGTH_SHORT 2
#define PRINTF_LENGTH_LONG 3
#define PRINTF_LENGTH_LONG_LONG 4
#endif