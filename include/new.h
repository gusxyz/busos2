#ifndef NEW_H
#define NEW_H
#include <stddef.h>
#include <liballoc.h>

inline void *operator new(size_t size)
{
    return kmalloc(size);
}

inline void *operator new[](size_t size)
{
    return kmalloc(size);
}

inline void operator delete(void *p)
{
    kfree(p);
}

inline void operator delete[](void *p)
{
    kfree(p);
}
#endif