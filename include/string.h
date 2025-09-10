#ifndef STRING_H
#define STRING_H

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif
    void *memcpy(void *__restrict, const void *__restrict, size_t);
    void *memmove(void *, const void *, size_t);
    void *memset(void *, int, size_t);
    int memcmp(const void *, const void *, size_t);
    size_t strlen(const char *);
    char *strtok_r(char *s, const char *delim, char **save_ptr);
    char *strdup(const char *s);
    size_t strspn(char *s, char *accept);
    size_t strcspn(char *s, char *reject);
    char *strchr(char *s, int c_in);
    int strcmp(char *p1, char *p2);

#ifdef __cplusplus
}
#endif
#endif
