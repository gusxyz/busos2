#include "util.h"

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
};

void memset(void *dest, char val, uint32_t count)
{
    char *temp = (char *)dest;
    for (; count != 0; count--)
    {
        *temp++ = val;
    }
};
