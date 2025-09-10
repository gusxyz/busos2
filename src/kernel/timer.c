#include <timer.h>
#include <idt.h>
#include <scheduler/scheduler.h>

uint64_t ticks = 0;
static const uint32_t freq = 1000; // 1 ms
bool schedulerEnabled;

void init_timer()
{
    irq_install_handler(0, onIrq0);

    // 1.1931816666 MHz -> 119318.16666 Hz
    uint32_t divisor = 1193180 / freq;

    // 0x43 = Mode/Command register
    // 0011 0110
    outb(0x43, 0x36);                             // -> Square Wave Generator
    outb(0x40, (uint8_t)(divisor & 0xFF));        // first chunk
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF)); // second chunk
}

void onIrq0(struct InterruptRegisters *reg)
{
    ticks++;
    if (schedulerEnabled)
    {
        scheduler_wake_sleeping_tasks(ticks);

        scheduler_tick();
    }
}

void sleep(uint32_t millis)
{
    task_sleep(millis);
}