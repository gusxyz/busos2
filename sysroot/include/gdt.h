#pragma once
#ifndef GDT_H
#define GDT_H
#include <stdint.h>

#define NUM_GDT_ENTRIES 6
#define GDT_KERNEL_CODE 0x08
#define GDT_KERNEL_DATA 0x10
#define GDT_USER_CODE 0x18
#define GDT_USER_DATA 0x20
#define GDT_TSS 0x28
#define RPL_USER 3

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct gdt_entry_struct
    {
        uint16_t limit;
        uint16_t base_low;
        uint8_t base_mid;
        uint8_t access;
        uint8_t flags;
        uint8_t base_hi;
    } __attribute__((packed)) gdt_entry_t;

    typedef struct gdt_ptr_struct
    {
        uint16_t limit;
        unsigned int base;
    } __attribute__((packed)) gdt_ptr_t;

    typedef struct tss_entry_struct
    {
        uint32_t prev_tss;
        uint32_t esp0;
        uint32_t ss0;
        uint32_t esp1;
        uint32_t ss1;
        uint32_t esp2;
        uint32_t ss2;
        uint32_t cr3;
        uint32_t eip3;
        uint32_t eflags;
        uint32_t eax;
        uint32_t edx;
        uint32_t ebx;
        uint32_t esp;
        uint32_t ebp;
        uint32_t esi;
        uint32_t edi;
        uint32_t es;
        uint32_t cs;
        uint32_t ss;
        uint32_t ds;
        uint32_t fs;
        uint32_t gs;
        uint32_t ldt;
        uint32_t trap;
        uint32_t iomap_base;
    } __attribute__((packed)) tss_entry_t;

    void initGDT();
    void setGDTGate(uint32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
    void writeTSS(uint32_t num, uint16_t ss0, uint32_t esp0);
    extern void gdt_flush(uint32_t);
    extern void tss_flush(uint32_t);
    extern tss_entry_t tss_entry;
#ifdef __cplusplus
}
#endif

#endif