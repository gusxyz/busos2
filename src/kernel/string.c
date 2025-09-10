#include <string.h>
#include <liballoc.h>

void *memcpy(void *__restrict dest, const void *__restrict src, size_t n)
{
    unsigned char *d = dest;
    const unsigned char *s = src;
    for (size_t i = 0; i < n; i++)
    {
        d[i] = s[i];
    }
    return dest;
}
void *memmove(void *dest, const void *src, size_t n)
{
    unsigned char *d = dest;
    const unsigned char *s = src;

    if (d < s)
    {
        // Copy from the beginning
        for (size_t i = 0; i < n; i++)
        {
            d[i] = s[i];
        }
    }
    else if (d > s)
    {
        // Copy from the end
        for (size_t i = n; i > 0; i--)
        {
            d[i - 1] = s[i - 1];
        }
    }
    return dest;
}

void *memset(void *s, int c, size_t n)
{
    unsigned char *p = s;
    unsigned char val = c;
    for (size_t i = 0; i < n; i++)
    {
        p[i] = val;
    }
    return s;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
    const unsigned char *p1 = s1;
    const unsigned char *p2 = s2;
    for (size_t i = 0; i < n; i++)
    {
        if (p1[i] != p2[i])
        {
            return p1[i] - p2[i];
        }
    }
    return 0;
}

size_t strlen(const char *s)
{
    size_t len = 0;
    while (s[len] != '\0')
    {
        len++;
    }
    return len;
}

char *strtok_r(char *s, const char *delim, char **save_ptr)
{
    char *end;
    if (s == NULL)
        s = *save_ptr;
    if (*s == '\0')
    {
        *save_ptr = s;
        return NULL;
    }
    /* Scan leading delimiters.  */
    s += strspn(s, delim);
    if (*s == '\0')
    {
        *save_ptr = s;
        return NULL;
    }
    /* Find the end of the token.  */
    end = s + strcspn(s, delim);
    if (*end == '\0')
    {
        *save_ptr = end;
        return s;
    }
    /* Terminate the token and make *SAVE_PTR point past it.  */
    *end = '\0';
    *save_ptr = end + 1;
    return s;
}

char *strdup(const char *s)
{
    size_t len = strlen(s) + 1;
    void *new = kmalloc(len);

    if (new == NULL)
        return NULL;

    return (char *)memcpy(new, s, len);
}
size_t strspn(char *s, char *accept)
{
    const char *p;
    const char *a;
    size_t count = 0;

    for (p = s; *p != '\0'; ++p)
    {
        for (a = accept; *a != '\0'; ++a)
            if (*p == *a)
                break;
        if (*a == '\0')
            return count;
        else
            ++count;
    }

    return count;
}

size_t strcspn(char *s, char *reject)
{
    size_t count = 0;

    while (*s != '\0')
        if (strchr(reject, *s++) == NULL)
            ++count;
        else
            return count;

    return count;
}

char *strchr(char *s, int c_in)
{
    const unsigned char *char_ptr;
    const unsigned long int *longword_ptr;
    unsigned long int longword, magic_bits, charmask;
    unsigned char c;

    c = (unsigned char)c_in;

    /* Handle the first few characters by reading one character at a time.
       Do this until CHAR_PTR is aligned on a longword boundary.  */
    for (char_ptr = (const unsigned char *)s;
         ((unsigned long int)char_ptr & (sizeof(longword) - 1)) != 0;
         ++char_ptr)
        if (*char_ptr == c)
            return (void *)char_ptr;
        else if (*char_ptr == '\0')
            return NULL;

    /* All these elucidatory comments refer to 4-byte longwords,
       but the theory applies equally well to 8-byte longwords.  */

    longword_ptr = (unsigned long int *)char_ptr;

    /* Bits 31, 24, 16, and 8 of this number are zero.  Call these bits
       the "holes."  Note that there is a hole just to the left of
       each byte, with an extra at the end:

       bits:  01111110 11111110 11111110 11111111
       bytes: AAAAAAAA BBBBBBBB CCCCCCCC DDDDDDDD

       The 1-bits make sure that carries propagate to the next 0-bit.
       The 0-bits provide holes for carries to fall into.  */
    switch (sizeof(longword))
    {
    case 4:
        magic_bits = 0x7efefeffL;
        break;
    case 8:
        magic_bits = ((0x7efefefeL << 16) << 16) | 0xfefefeffL;
        break;
    default:
        return NULL;
    }

    /* Set up a longword, each of whose bytes is C.  */
    charmask = c | (c << 8);
    charmask |= charmask << 16;
    if (sizeof(longword) > 4)
        /* Do the shift in two steps to avoid a warning if long has 32 bits.  */
        charmask |= (charmask << 16) << 16;
    if (sizeof(longword) > 8)
        return NULL;

    /* Instead of the traditional loop which tests each character,
       we will test a longword at a time.  The tricky part is testing
       if *any of the four* bytes in the longword in question are zero.  */
    for (;;)
    {
        /* We tentatively exit the loop if adding MAGIC_BITS to
       LONGWORD fails to change any of the hole bits of LONGWORD.

       1) Is this safe?  Will it catch all the zero bytes?
       Suppose there is a byte with all zeros.  Any carry bits
       propagating from its left will fall into the hole at its
       least significant bit and stop.  Since there will be no
       carry from its most significant bit, the LSB of the
       byte to the left will be unchanged, and the zero will be
       detected.

       2) Is this worthwhile?  Will it ignore everything except
       zero bytes?  Suppose every byte of LONGWORD has a bit set
       somewhere.  There will be a carry into bit 8.  If bit 8
       is set, this will carry into bit 16.  If bit 8 is clear,
       one of bits 9-15 must be set, so there will be a carry
       into bit 16.  Similarly, there will be a carry into bit
       24.  If one of bits 24-30 is set, there will be a carry
       into bit 31, so all of the hole bits will be changed.

       The one misfire occurs when bits 24-30 are clear and bit
       31 is set; in this case, the hole at bit 31 is not
       changed.  If we had access to the processor carry flag,
       we could close this loophole by putting the fourth hole
       at bit 32!

       So it ignores everything except 128's, when they're aligned
       properly.

       3) But wait!  Aren't we looking for C as well as zero?
       Good point.  So what we do is XOR LONGWORD with a longword,
       each of whose bytes is C.  This turns each byte that is C
       into a zero.  */

        longword = *longword_ptr++;

        /* Add MAGIC_BITS to LONGWORD.  */
        if ((((longword + magic_bits)

              /* Set those bits that were unchanged by the addition.  */
              ^ ~longword)

             /* Look at only the hole bits.  If any of the hole bits
                are unchanged, most likely one of the bytes was a
                zero.  */
             & ~magic_bits) != 0 ||

            /* That caught zeroes.  Now test for C.  */
            ((((longword ^ charmask) + magic_bits) ^ ~(longword ^ charmask)) & ~magic_bits) != 0)
        {
            /* Which of the bytes was C or zero?
               If none of them were, it was a misfire; continue the search.  */

            const unsigned char *cp = (const unsigned char *)(longword_ptr - 1);

            if (*cp == c)
                return (char *)cp;
            else if (*cp == '\0')
                return NULL;
            if (*++cp == c)
                return (char *)cp;
            else if (*cp == '\0')
                return NULL;
            if (*++cp == c)
                return (char *)cp;
            else if (*cp == '\0')
                return NULL;
            if (*++cp == c)
                return (char *)cp;
            else if (*cp == '\0')
                return NULL;
            if (sizeof(longword) > 4)
            {
                if (*++cp == c)
                    return (char *)cp;
                else if (*cp == '\0')
                    return NULL;
                if (*++cp == c)
                    return (char *)cp;
                else if (*cp == '\0')
                    return NULL;
                if (*++cp == c)
                    return (char *)cp;
                else if (*cp == '\0')
                    return NULL;
                if (*++cp == c)
                    return (char *)cp;
                else if (*cp == '\0')
                    return NULL;
            }
        }
    }

    return NULL;
}

int strcmp(char *p1, char *p2)
{
    register const unsigned char *s1 = (const unsigned char *)p1;
    register const unsigned char *s2 = (const unsigned char *)p2;
    unsigned char c1, c2;

    do
    {
        c1 = (unsigned char)*s1++;
        c2 = (unsigned char)*s2++;
        if (c1 == '\0')
            return c1 - c2;
    } while (c1 == c2);

    return c1 - c2;
}
