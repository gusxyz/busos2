#include "idt.h"
#include "../vga.h"
#include "../stdlib/stdio.h"

struct idt_entry_struct idt_entries[256];
struct idt_ptr_struct idt_ptr;

void initIDT()
{
    idt_ptr.limit = sizeof(struct idt_entry_struct) * 256 - 1;
    idt_ptr.base = (uint32_t)&idt_entries;

    memset(&idt_entries, 0, sizeof(struct idt_entry_struct) * 256);

    // 0x20 commands 0x21 data
    // 0xA0 commands 0XA1 data

    // initialize PICs
    outb(0x20, 0x11);
    outb(0xA0, 0x11);

    outb(0x21, 0x20);
    outb(0xA1, 0x28);

    outb(0x21, 0x04);
    outb(0xA1, 0x02);

    outb(0x21, 0x01);
    outb(0xA1, 0x01);

    outb(0x21, 0x0);
    outb(0xA1, 0x0);

    setIDTGate(0, (uint32_t)isr0, 0x08, 0x8E);
    setIDTGate(1, (uint32_t)isr1, 0x08, 0x8E);
    setIDTGate(2, (uint32_t)isr2, 0x08, 0x8E);
    setIDTGate(3, (uint32_t)isr3, 0x08, 0x8E);
    setIDTGate(4, (uint32_t)isr4, 0x08, 0x8E);
    setIDTGate(5, (uint32_t)isr5, 0x08, 0x8E);
    setIDTGate(6, (uint32_t)isr6, 0x08, 0x8E);
    setIDTGate(7, (uint32_t)isr7, 0x08, 0x8E);
    setIDTGate(8, (uint32_t)isr8, 0x08, 0x8E);
    setIDTGate(9, (uint32_t)isr9, 0x08, 0x8E);
    setIDTGate(10, (uint32_t)isr10, 0x08, 0x8E);
    setIDTGate(11, (uint32_t)isr11, 0x08, 0x8E);
    setIDTGate(12, (uint32_t)isr12, 0x08, 0x8E);
    setIDTGate(13, (uint32_t)isr13, 0x08, 0x8E);
    setIDTGate(14, (uint32_t)isr14, 0x08, 0x8E);
    setIDTGate(15, (uint32_t)isr15, 0x08, 0x8E);
    setIDTGate(16, (uint32_t)isr16, 0x08, 0x8E);
    setIDTGate(17, (uint32_t)isr17, 0x08, 0x8E);
    setIDTGate(18, (uint32_t)isr18, 0x08, 0x8E);
    setIDTGate(19, (uint32_t)isr19, 0x08, 0x8E);
    setIDTGate(20, (uint32_t)isr20, 0x08, 0x8E);
    setIDTGate(21, (uint32_t)isr21, 0x08, 0x8E);
    setIDTGate(22, (uint32_t)isr22, 0x08, 0x8E);
    setIDTGate(23, (uint32_t)isr23, 0x08, 0x8E);
    setIDTGate(24, (uint32_t)isr24, 0x08, 0x8E);
    setIDTGate(25, (uint32_t)isr25, 0x08, 0x8E);
    setIDTGate(26, (uint32_t)isr26, 0x08, 0x8E);
    setIDTGate(27, (uint32_t)isr27, 0x08, 0x8E);
    setIDTGate(28, (uint32_t)isr28, 0x08, 0x8E);
    setIDTGate(29, (uint32_t)isr29, 0x08, 0x8E);
    setIDTGate(30, (uint32_t)isr30, 0x08, 0x8E);
    setIDTGate(31, (uint32_t)isr31, 0x08, 0x8E);

    setIDTGate(32, (uint32_t)irq0, 0x08, 0x8E);
    setIDTGate(33, (uint32_t)irq1, 0x08, 0x8E);
    setIDTGate(34, (uint32_t)irq2, 0x08, 0x8E);
    setIDTGate(35, (uint32_t)irq3, 0x08, 0x8E);
    setIDTGate(36, (uint32_t)irq4, 0x08, 0x8E);
    setIDTGate(37, (uint32_t)irq5, 0x08, 0x8E);
    setIDTGate(38, (uint32_t)irq6, 0x08, 0x8E);
    setIDTGate(39, (uint32_t)irq7, 0x08, 0x8E);
    setIDTGate(40, (uint32_t)irq8, 0x08, 0x8E);
    setIDTGate(41, (uint32_t)irq9, 0x08, 0x8E);
    setIDTGate(42, (uint32_t)irq10, 0x08, 0x8E);
    setIDTGate(43, (uint32_t)irq11, 0x08, 0x8E);
    setIDTGate(44, (uint32_t)irq12, 0x08, 0x8E);
    setIDTGate(45, (uint32_t)irq13, 0x08, 0x8E);
    setIDTGate(46, (uint32_t)irq14, 0x08, 0x8E);
    setIDTGate(47, (uint32_t)irq15, 0x08, 0x8E);

    setIDTGate(128, (uint32_t)isr128, 0x08, 0x8E); // sys calls
    setIDTGate(128, (uint32_t)isr177, 0x08, 0x8E); // sys calls

    idt_flush((uint32_t)&idt_ptr);
    serial_putsf("IDT Initialized.\n");
}

void setIDTGate(uint8_t num, uint32_t base, uint16_t selector, uint8_t flags)
{
    idt_entries[num].base_low = base & 0xFFFF;
    idt_entries[num].base_high = (base >> 16) & 0xFFFF;
    idt_entries[num].selector = selector;
    idt_entries[num].always0 = 0;
    idt_entries[num].flags = flags | 0x60;
}

const char *exceptionMessages[] = {
    "Division Error",
    "Debug Exception",
    "Nonmaskable external interrupt",
    "Breakpoint",
    "INTO Detected Overflow",
    "BOUND Range Exceeded",
    "Invalid Opcode (Undefined Opcode)",
    "Device Not Avaliable",
    "Double Fault",
    "Invalid TSS Segment",
    "Segment not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check Fault",
    "Machine Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"};
void handle_page_fault(struct InterruptRegisters *regs)
{
    uint32_t faulting_address;
    uint32_t error_code = regs->err_code;

    // Get the faulting address from CR2 register
    asm volatile("mov %%cr2, %0" : "=r"(faulting_address));

    // Parse the error code bits
    uint8_t present = error_code & 0x1;
    uint8_t write = (error_code >> 1) & 0x1;
    uint8_t user = (error_code >> 2) & 0x1;
    uint8_t reserved = (error_code >> 3) & 0x1;
    uint8_t instruction = (error_code >> 4) & 0x1;
    uint8_t protection_key = (error_code >> 5) & 0x1;
    uint8_t shadow_stack = (error_code >> 6) & 0x1;
    uint8_t sgx = (error_code >> 15) & 0x1;

    // Print readable error message
    serial_putsf("Page Fault Exception (#PF)\n");
    serial_putsf("Faulting Address: 0x%08X\n", faulting_address);
    serial_putsf("Error Code: 0x%04X\n", error_code);

    // Basic cause
    serial_putsf("Cause: ");
    if (!present)
    {
        serial_putsf("Non-present page");
    }
    else
    {
        serial_putsf("Protection violation");
    }
    serial_putsf("\n");

    // Operation details
    serial_putsf("Operation: ");
    if (instruction)
    {
        serial_putsf("Instruction fetch");
    }
    else
    {
        serial_putsf("Data access");
    }
    if (write)
    {
        serial_putsf(" (write)");
    }
    else
    {
        serial_putsf(" (read)");
    }
    serial_putsf("\n");

    // Privilege level
    serial_putsf("Privilege: ");
    if (user)
    {
        serial_putsf("User mode");
    }
    else
    {
        serial_putsf("Supervisor mode");
    }
    serial_putsf("\n");

    // Additional flags
    if (reserved)
    {
        serial_putsf("Reserved bit was set in page structure\n");
    }
    if (protection_key)
    {
        serial_putsf("Protection key violation\n");
    }
    if (shadow_stack)
    {
        serial_putsf("Shadow stack access fault\n");
    }
    if (sgx)
    {
        serial_putsf("SGX violation\n");
    }

    // Register state
    serial_putsf("Register state at time of fault:\n");
    serial_putsf("EIP: 0x%08X\n", regs->eip);

    // Traditional interpretation table
    // serial_putsf("\nTraditional interpretation:\n");
    // serial_putsf("%s process tried to %s a %s\n",
    //       user ? "User" : "Supervisory",
    //       write ? "write" : "read",
    //       present ? "page and caused a protection fault" : "non-present page entry");

    // serial_putsf("\nSystem Halted. Debug information above.\n");

    // Halt the system
    for (;;)
        ;
}

void isr_handler(struct InterruptRegisters *registers)
{
    if (registers->int_no == 14)
    {
        handle_page_fault(registers);
    }
    else if (registers->int_no < 32)
    {
        serial_putsf(exceptionMessages[registers->int_no]);
        serial_putsf("\n");
        serial_putsf("Exception Reached.\nSystem Haulted\n");
        for (;;)
            ;
    }
}

void *irq_routines[16] =
    {
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0};

void irq_install_handler(int irq, void (*handler)(struct InterruptRegisters *r))
{
    irq_routines[irq] = handler;
}

void irq_uninstall_handler(int irq)
{
    irq_routines[irq] = 0;
}

void irq_handler(struct InterruptRegisters *registers)
{
    void (*handler)(struct InterruptRegisters *registers);
    handler = irq_routines[registers->int_no - 32];

    if (handler)
    {
        handler(registers);
    }

    if (registers->int_no >= 40)
    {
        outb(0xA0, 0x20);
    }

    outb(0x20, 0x20);
}