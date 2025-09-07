#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef volatile bool spinlock_t;

static inline void spinlock_acquire(spinlock_t *lock)
{
    // Atomic GCC built-in.
    while (__sync_lock_test_and_set(lock, 1))
    {
        asm volatile("pause");
    }
}

static inline void spinlock_release(spinlock_t *lock)
{
    // Atomic GCC built-in.
    __sync_lock_release(lock);
}
