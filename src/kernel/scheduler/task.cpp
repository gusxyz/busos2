#include <scheduler/task.hpp>
#include <gdt.h>
#include <stdio.h>

/// @brief Defined in scheduler.s
extern "C" void newTaskSetup();

extern tss_entry_t tss_entry;

Task::Task(uint32_t id, uint32_t entry_point, uint32_t kernel_stack_top, bool is_kernel)
{
    if (entry_point == 0)
    {
        printf("!!! PANIC: Attempted to create task T%d with a NULL entry point !!!\n", id);
        for (;;)
            asm("cli; hlt");
    }

    this->id = id;
    this->kesp_bottom = kernel_stack_top;
    this->state = TaskState::RUNNING;
    uint32_t code_selector = is_kernel ? GDT_KERNEL_CODE : (GDT_USER_CODE | 3);
    uint32_t data_selector = is_kernel ? GDT_KERNEL_DATA : (GDT_USER_DATA | 3);

    uint8_t *kesp_ptr = (uint8_t *)kernel_stack_top;
    kesp_ptr -= sizeof(NewTaskKernelStack);
    NewTaskKernelStack *stack = (NewTaskKernelStack *)kesp_ptr;

    stack->ebp = stack->edi = stack->esi = stack->ebx = 0;
    stack->switch_context_return_addr = (uint32_t)newTaskSetup;
    stack->ds = data_selector;
    stack->eip = entry_point;
    stack->cs = code_selector;
    stack->eflags = 0x202;

    stack->usermode_esp = 0;
    stack->usermode_ss = 0;

    this->kesp = (uint32_t)kesp_ptr;
}

Task::Task(uint32_t id, bool is_kernel)
{
    this->id = id;
    this->kesp = 0;
    this->kesp_bottom = 0;
}

void Task::set_tss_stack(uint32_t stack)
{
    tss_entry.esp0 = stack;
}