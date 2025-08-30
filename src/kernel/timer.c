#include "timer.h"
#include "idt/idt.h"

uint32_t countDown = 0;
uint64_t ticks;
const uint32_t freq = 100;

void initTimer()
{
    ticks = 0;
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
    ticks += 1;
    if (countDown > 0)
        countDown -= 1;
}
void sleep(uint32_t millis)
{
    countDown = millis;
    while (countDown > 0)
    {
        __asm__ volatile("hlt");
    }
}