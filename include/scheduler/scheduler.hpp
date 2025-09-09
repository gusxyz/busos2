#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP
#include <scheduler/task.hpp> // Use the .hpp for the C++ Task class
#include <stdint.h>

#define MAX_TASKS 16 // <<<<<<< DEFINE IT HERE
extern "C" void contextSwitch(Task *from, Task *to);

class Scheduler
{
private:
    Task *tasks[MAX_TASKS];
    int num_tasks;
    int current_task_index;

public:
    Scheduler();
    void init();
    void createTask(uint32_t entry_point, bool is_kernel_task);
    void schedule();
    Task *getCurrentTask();
    void wakeSleepingTasks(uint64_t current_ticks);
};
#endif