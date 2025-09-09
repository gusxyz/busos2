#include <scheduler/scheduler.hpp>
#include <scheduler/scheduler.h>
#include <stdio.h>
#include <new.h>
#include <liballoc.h>
#include <timer.h>
#include <stdio.h>

Scheduler::Scheduler() : num_tasks(0), current_task_index(-1) {}

void Scheduler::init()
{
    tasks[0] = new Task(0, true);
    num_tasks = 1;
    current_task_index = 0;
}

Task *Scheduler::getCurrentTask()
{
    if (current_task_index < 0)
        return nullptr;
    return tasks[current_task_index];
}

void Scheduler::createTask(uint32_t entry_point, bool is_kernel_task)
{
    if (num_tasks >= MAX_TASKS)
    {
        printf("PANIC: Exceeded MAX_TASKS!\n");
        for (;;)
            asm("cli; hlt");
    }

    if (entry_point == 0)
    {
        printf("PANIC: scheduler_create_task called with a NULL entry point!\n");
        for (;;)
            asm("cli; hlt");
    }

    if (num_tasks >= MAX_TASKS)
        return;

    const uint32_t stack_size = 16384;
    void *stack_memory = kmalloc(stack_size);
    uint32_t stack_top = (uint32_t)stack_memory + stack_size;

    tasks[num_tasks] = new Task(num_tasks, entry_point, stack_top, is_kernel_task);
    if (tasks[num_tasks] == nullptr)
    {
        printf("PANIC: Failed to allocate memory for new task T%d!\n", num_tasks);
        // We could also free the stack_memory here.
        for (;;)
            asm("cli; hlt");
    }
    num_tasks++;
}

void Scheduler::wakeSleepingTasks(uint64_t current_ticks)
{
    for (int i = 0; i < num_tasks; ++i)
    {
        if (tasks[i]->state == TaskState::SLEEPING && current_ticks >= tasks[i]->wake_at_tick)
        {
            tasks[i]->setState(TaskState::RUNNING);
        }
    }
}

void Scheduler::schedule()
{
    if (num_tasks <= 1)
        return;

    Task *old_task = tasks[current_task_index];
    // <<<<<<<<<<<< THIS IS THE NEW, SMARTER LOGIC >>>>>>>>>>>>>
    // Find the NEXT runnable task
    int next_task_index = current_task_index;
    do
    {
        next_task_index = (next_task_index + 1) % num_tasks;
    } while (tasks[next_task_index]->state != TaskState::RUNNING);

    // If we looped all the way around and only found ourselves,
    // and we aren't runnable, then halt (nothing to do).
    // This is a basic idle check.
    if (next_task_index == current_task_index && old_task->state != TaskState::RUNNING)
    {
        // In a real OS, you'd find a dedicated idle task. For now, we just halt.
        asm volatile("hlt");
        return;
    }

    current_task_index = (current_task_index + 1) % num_tasks;

    Task *new_task = tasks[current_task_index];
    if (new_task == nullptr)
    {
        printf("PANIC: Scheduler selected a NULL task to run (T%d)!\n", current_task_index);
        for (;;)
            asm("cli; hlt");
    }

    contextSwitch(old_task, new_task);
}

static Scheduler scheduler_instance;

// c functions
extern "C" bool schedulerEnabled;

extern "C" void scheduler_init()
{
    schedulerEnabled = true;
    scheduler_instance.init();
}

extern "C" void scheduler_create_task(uint32_t entry_point, bool is_kernel_task)
{
    scheduler_instance.createTask(entry_point, is_kernel_task);
}

extern "C" void scheduler_tick()
{
    scheduler_instance.schedule();
}

extern "C" void task_sleep(uint32_t milliseconds)
{
    Task *current_task = scheduler_instance.getCurrentTask();
    if (!current_task)
        return;
    // Call C++ methods on the C++ object
    current_task->setWakeTime(ticks + milliseconds); // We need to add setWakeTime
    current_task->setState(TaskState::SLEEPING);     // We need to add setState

    // Immediately trigger a context switch
    scheduler_tick();
}

extern "C" void scheduler_wake_sleeping_tasks(uint64_t current_ticks)
{
    // Call a C++ method on the scheduler instance to do the work
    scheduler_instance.wakeSleepingTasks(current_ticks);
}
