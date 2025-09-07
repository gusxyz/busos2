#pragma once
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>
#define TASK_STATE_RUNNING 0
#define TASK_STATE_SLEEPING 1

#ifdef __cplusplus
extern "C"
{
#endif
    extern bool schedulerEnabled;
    void scheduler_init();

    void scheduler_create_task(uint32_t entry_point, bool is_kernel_task);

    void scheduler_wake_sleeping_tasks(uint64_t current_ticks);
    void task_sleep(uint32_t milliseconds);
    void scheduler_tick();

#ifdef __cplusplus
}
#endif

#endif
