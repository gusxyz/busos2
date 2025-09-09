#ifndef TASK_HPP
#define TASK_HPP
#include <stdint.h>
#include <scheduler/scheduler.h>
#include <new.h>

enum class TaskState
{
    RUNNING,
    SLEEPING,
    BLOCKED, // For later (e.g., waiting for disk)
    DEAD
};

extern "C" struct NewTaskKernelStack
{
    // Callee-saved registers
    uint32_t ebp, edi, esi, ebx;

    // Popped by ret in contextSwitch
    uint32_t switch_context_return_addr;

    // Popped by us in new_task_setup
    uint32_t ds; // <<<<<<< ADD THIS LINE

    // Popped by iret in new_task_setup
    uint32_t eip, cs, eflags, usermode_esp, usermode_ss;
};

class Task
{
public:
    // --- ASSEMBLY-VISIBLE MEMBERS FIRST ---
    uint32_t kesp;        // Offset +0
    uint32_t id;          // Offset +4
    uint32_t kesp_bottom; // Offset +8

    // --- C++ ONLY MEMBERS ---
    TaskState state;
    uint64_t wake_at_tick;

    Task(uint32_t id, uint32_t entry_point, uint32_t kernel_stack_top, bool is_kernel);
    Task(uint32_t id, bool is_kernel);

    // This was missing from the class definition
    void set_tss_stack(uint32_t stack); // <<<<<<< FIX #2: ADD THIS DECLARATION
    void setState(TaskState new_state) { state = new_state; }
    void setWakeTime(uint64_t ticks) { wake_at_tick = ticks; }
};
#endif