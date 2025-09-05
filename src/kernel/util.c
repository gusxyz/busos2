#include <util.h>
#include <stdio.h>

void *memset(void *s, int c, size_t n)
{
    unsigned char *p = (unsigned char *)s;
    unsigned char value = (unsigned char)c;

    for (size_t i = 0; i < n; i++)
        p[i] = value;

    return s;
};

void *memcpy(void *destination, void *source, size_t num)
{
    int i;
    char *d = destination;
    char *s = source;
    for (i = 0; i < num; i++)
        d[i] = s[i];

    return destination;
}

int strlen(char *s)
{
    int len = 0;
    while (*s != '\0')
    {
        len++;
        s++;
    }
    return len;
}

// Initialize serial port (COM1)
void serialInit()
{
    outb(0x3F8 + 1, 0x00); // Disable interrupts
    outb(0x3F8 + 3, 0x80); // Enable DLAB
    outb(0x3F8 + 0, 0x03); // Divisor low byte (115200 baud)
    outb(0x3F8 + 1, 0x00); // Divisor high byte
    outb(0x3F8 + 3, 0x03); // 8 bits, no parity, one stop bit
    outb(0x3F8 + 2, 0xC7); // Enable FIFO, clear, 14-byte threshold
    outb(0x3F8 + 4, 0x0B); // IRQs enabled, RTS/DSR set
}

// Check if serial transmit is empty
int serialTransmitEmpty()
{
    return inb(0x3F8 + 5) & 0x20;
}

// Write a character to serial port
void serial_putc(char c)
{
    while (serialTransmitEmpty() == 0)
        ;
    outb(0x3F8, c);
}

// Write a string to serial port
void serial_puts(const char *str)
{
    while (*str)
    {
        serial_putc(*str++);
    }
}

void serial_putsf(const char *fmt, ...)
{
    int *argp = (int *)&fmt;
    int state = PRINTF_STATE_START;
    int length = PRINTF_LENGTH_START;
    int radix = 10;
    bool sign = false;

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
                serial_putc(*fmt);
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
                serial_putc((char)*argp);
                argp++;
                break;
            case 's':
                if (length == PRINTF_LENGTH_LONG || length == PRINTF_LENGTH_LONG_LONG)
                {
                    serial_puts(*(const char **)argp);
                    argp += 2;
                }
                else
                {
                    serial_puts(*(const char **)argp);
                    argp++;
                }
                break;
            case '%':
                serial_putc('%');
                break;
            case 'd':
            case 'i':
                radix = 10;
                sign = true;
                argp = serial_putsfn(argp, length, sign, radix);
                break;
            case 'u':
                radix = 10;
                sign = false;
                argp = serial_putsfn(argp, length, sign, radix);
                break;
            case 'X':
            case 'x':
            case 'p':
                radix = 16;
                sign = false;
                argp = serial_putsfn(argp, length, sign, radix);
                break;
            case 'o':
                radix = 8;
                sign = false;
                argp = serial_putsfn(argp, length, sign, radix);
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

int *serial_putsfn(int *argp, int length, bool sign, int radix)
{
    char buffer[32] = "";
    uint32_t number;
    int number_sign = 1;
    int pos = 0;

    switch (length)
    {
    case PRINTF_LENGTH_SHORT_SHORT:
    case PRINTF_LENGTH_SHORT:
    case PRINTF_LENGTH_START:
        if (sign)
        {
            int n = *argp;
            if (n < 0)
            {
                n = -n;
                number_sign = -1;
            }
            number = (uint32_t)n;
        }
        else
        {
            number = *(uint32_t *)argp;
        }
        argp++;
        break;
    case PRINTF_LENGTH_LONG:
        if (sign)
        {
            long int n = *(long int *)argp;
            if (n < 0)
            {
                n = -n;
                number_sign = -1;
            }
            number = (uint32_t)n;
        }
        else
        {
            number = *(uint32_t *)argp;
        }
        argp += 2;
        break;
    case PRINTF_LENGTH_LONG_LONG:
        if (sign)
        {
            long long int n = *(long long int *)argp;
            if (n < 0)
            {
                n = -n;
                number_sign = -1;
            }
            number = (uint32_t)n;
        }
        else
        {
            number = *(uint32_t *)argp;
        }
        argp += 4;
        break;
    }

    do
    {
        uint32_t rem = number % radix;
        number = number / radix;

        buffer[pos++] = possibleChars[rem];
    } while (number > 0);

    if (sign && number_sign < 0)
    {
        buffer[pos++] = '-';
    }

    while (--pos >= 0)
    {
        serial_putc(buffer[pos]);
    }

    return argp;
}

int memcmp(const void *__s1, const void *__s2, size_t __n)
{
    unsigned char *a = (unsigned char *)__s1, *b = (unsigned char *)__s2;
    if (__n > 0)
    {
        while (__n-- > 0)
        {
            if (*a != *b)
            {
                return *a - *b;
            }
            a++;
            b++;
        }
    }
    return 0;
}