#include <stdio.h>
#include <console.h>
#include <spinlock.h>

const char possibleChars[] = "0123456789ABCDEF";
extern spinlock_t console_lock;

void putc(char c)
{
    consolePutC(c); // Print the valid, null-terminated string
}

void puts(const char *s)
{
    while (*s)
    {
        putc(*s);
        s++;
    }
}

void vprintf(const char *fmt, va_list args)
{
    int *argp = (int *)&fmt;
    int state = PRINTF_STATE_START;
    int length = PRINTF_LENGTH_START;
    int radix = 10;
    bool sign = false;
    va_list argsCopy;

    va_copy(argsCopy, args);

    argp++;
    while (*fmt)
    {
        switch (state)
        {
        case PRINTF_STATE_START:
            if (*fmt == '%')
            {
                state = PRINTF_STATE_LENGTH;
            }
            else
            {
                putc(*fmt);
            }
            break;
        case PRINTF_STATE_LENGTH:
            if (*fmt == 'h')
            {
                length = PRINTF_LENGTH_SHORT;
                state = PRINTF_STATE_SHORT;
            }
            else if (*fmt == 'l')
            {
                length = PRINTF_LENGTH_LONG;
                state = PRINTF_STATE_LONG;
            }
            else
            {
                goto PRINTF_STATE_SPEC_;
            }
            break;
            // hd
        case PRINTF_STATE_SHORT:
            if (*fmt == 'h')
            {
                length = PRINTF_LENGTH_SHORT_SHORT;
                state = PRINTF_STATE_SPEC;
            }
            else
            {
                goto PRINTF_STATE_SPEC_;
            }
            break;

        case PRINTF_STATE_LONG:
            if (*fmt == 'l')
            {
                length = PRINTF_LENGTH_LONG_LONG;
                state = PRINTF_STATE_SPEC;
            }
            else
            {
                goto PRINTF_STATE_SPEC_;
            }
            break;

        case PRINTF_STATE_SPEC:
        PRINTF_STATE_SPEC_:
            switch (*fmt)
            {
            case 'c':
                putc((char)va_arg(argsCopy, int));
                break;
            case 's':
                puts(va_arg(argsCopy, const char *));
                break;
            case '%':
                putc('%');
                break;
            case 'd':
            case 'i':
                radix = 10;
                sign = true;
                printf_number_va(argsCopy, length, sign, radix);
                break;
            case 'u':
                radix = 10;
                sign = false;
                printf_number_va(argsCopy, length, sign, radix);
                break;
            case 'X':
            case 'x':
            case 'p':
                radix = 16;
                sign = false;
                printf_number_va(argsCopy, length, sign, radix);
                break;
            case 'o':
                radix = 8;
                sign = false;
                printf_number_va(argsCopy, length, sign, radix);
                break;
            default:
                break;
            }
            state = PRINTF_STATE_START;
            length = PRINTF_LENGTH_START;
            radix = 10;
            sign = false;
            break;
        }
        fmt++;
    }
}

void printf(const char *fmt, ...)
{
    spinlock_acquire(&console_lock);

    // 2. We need to handle variadic arguments correctly.
    va_list args;
    va_start(args, fmt);
    // Call a 'vprintf' version that uses the va_list.
    // You will likely need to create this from your existing printf logic.
    // For simplicity, let's assume your printf can be called like this for now.
    // A proper vprintf implementation is the robust solution.
    // If you don't have one, we can adapt your existing printf.
    vprintf(fmt, args);
    va_end(args);

    // 3. Release the lock so other tasks can print.
    spinlock_release(&console_lock);
}

void printf_number_va(va_list args, int length, bool sign, int radix)
{
    uint32_t value_unsigned;
    int32_t value_signed;
    char buffer[32]; // Enough for 32-bit numbers in any base
    char *ptr = buffer;

    // Extract the appropriate value based on length and sign
    switch (length)
    {
    case PRINTF_LENGTH_SHORT_SHORT: // char
        if (sign)
        {
            value_signed = (int32_t)(signed char)va_arg(args, int);
        }
        else
        {
            value_unsigned = (uint32_t)(unsigned char)va_arg(args, int);
        }
        break;

    case PRINTF_LENGTH_SHORT: // short
        if (sign)
        {
            value_signed = (int32_t)(short)va_arg(args, int);
        }
        else
        {
            value_unsigned = (uint32_t)(unsigned short)va_arg(args, int);
        }
        break;

    case PRINTF_LENGTH_LONG: // long (32-bit on 32-bit systems)
        if (sign)
        {
            value_signed = (int32_t)va_arg(args, long);
        }
        else
        {
            value_unsigned = (uint32_t)va_arg(args, unsigned long);
        }
        break;

    case PRINTF_LENGTH_LONG_LONG: // long long (64-bit, but we'll handle as 32-bit for compatibility)
        // On 32-bit systems, you might want to implement proper 64-bit support
        // or limit to 32-bit values. For simplicity, we'll treat as 32-bit:
        if (sign)
        {
            value_signed = (int32_t)va_arg(args, long long);
        }
        else
        {
            value_unsigned = (uint32_t)va_arg(args, unsigned long long);
        }
        break;

    default: // int (default)
        if (sign)
        {
            value_signed = (int32_t)va_arg(args, int);
        }
        else
        {
            value_unsigned = (uint32_t)va_arg(args, unsigned int);
        }
        break;
    }

    // Convert number to string based on sign
    if (sign)
    {
        if (value_signed < 0)
        {
            putc('-');
            value_unsigned = (uint32_t)(-value_signed);
        }
        else
        {
            value_unsigned = (uint32_t)value_signed;
        }
    }

    // Convert to string in the specified radix
    do
    {
        uint32_t digit = value_unsigned % radix;
        value_unsigned /= radix;
        *ptr++ = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
    } while (value_unsigned > 0);

    // Print the string in reverse order
    while (ptr > buffer)
    {
        putc(*--ptr);
    }
}
int strcmp(const char *s1, const char *s2)
{
    unsigned char c1, c2;
    while ((c1 = *s1++) == (c2 = *s2++))
    {
        if (c1 == '\0')
            return 0;
    }
    return c1 - c2;
}
