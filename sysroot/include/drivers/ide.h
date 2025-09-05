#pragma once
#include <drivers/pci.h>
#include <util.h>
#include <stdio.h>
#include <drivers/atadefs.h>

typedef struct ide_channel_register
{
   uint16_t base;  // I/O Base.
   uint16_t ctrl;  // Control Base
   uint16_t bmide; // Bus Master IDE
   uint8_t nIEN;   // nIEN (No Interrupt);
} ide_channel_register_t;

typedef struct ide_device
{
   uint8_t Reserved;         // 0 (Empty) or 1 (This Drive really exists).
   uint8_t Channel;          // 0 (Primary Channel) or 1 (Secondary Channel).
   uint8_t Drive;            // 0 (Master Drive) or 1 (Slave Drive).
   uint16_t Type;            // 0: ATA, 1:ATAPI.
   uint16_t Signature;       // Drive Signature
   uint16_t Capabilities;    // Features.
   unsigned int CommandSets; // Command Sets Supported.
   unsigned int Size;        // Size in Sectors.
   uint8_t Model[41];        // Model in string.
} ide_device_t;

void initIDEController();
void idePrintProg(pci_device_t *device);
void ideInitialize(unsigned int BAR0, unsigned int BAR1, unsigned int BAR2, unsigned int BAR3, unsigned int BAR4);
uint8_t ideRead(uint8_t channel, uint8_t register);
void ideWrite(uint8_t channel, uint8_t reg, uint8_t data);
uint8_t idePrintError(unsigned int drive, uint8_t err);
void ideReadBuffer(uint8_t channel, uint8_t reg, unsigned int buffer, unsigned int quads);
uint8_t idePolling(uint8_t channel, unsigned int advanced_check);
void ideWaitIrq();
void ideIrq();
uint8_t ideAtapiRead(uint8_t drive, unsigned int lba, uint8_t numsects, unsigned short selector, unsigned int edi);
void ideIrqHandle(struct InterruptRegisters *r);

// inlines
static inline void atapiReadSector(uint16_t selector, void *edi, uint16_t bus, int words)
{
   __asm__ volatile(
       "pushw %%es\n\t"
       "movw %3, %%es\n\t"
       "rep insw\n\t"
       "popw %%es"
       : "+D"(edi), "+c"(words)
       : "d"(bus), "a"(selector)
       : "memory");
}