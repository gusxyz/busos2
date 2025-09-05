#pragma once
#include <stdint.h>
#include <stdbool.h>

#define MAX_PCI_DEVICES 32

typedef struct pci_device
{
    uint8_t bus;
    uint8_t slot;
    uint8_t func;

    uint16_t vendor_id;
    uint16_t device_id;

    uint8_t class_code;
    uint8_t subclass;
    uint8_t header_type;

    uint8_t prog_if;

    uint32_t bar0;
    uint32_t bar1;
    uint32_t bar2;
    uint32_t bar3;
    uint32_t bar4;
    uint32_t bar5;
} pci_device_t;

void pciInit(bool dumpPCI);
uint32_t pciConfigReadDWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint16_t pciConfigReadWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint16_t pciGetVendorID(uint8_t bus, uint8_t device, uint8_t func);
uint16_t pciGetDeviceID(uint8_t bus, uint8_t device, uint8_t func);
uint8_t pciGetClass(uint8_t bus, uint8_t device, uint8_t func);
uint8_t pciGetSubclass(uint8_t bus, uint8_t device, uint8_t func);
uint8_t pciGetHeaderType(uint8_t bus, uint8_t device, uint8_t func);
uint8_t pciGetProgIF(uint8_t bus, uint8_t device, uint8_t func);
void checkDevice(uint8_t bus, uint8_t device);
void checkAllBuses();
void checkBus(uint8_t bus);
void checkFunction(uint8_t bus, uint8_t slot, uint8_t func);
pci_device_t *pciFindDevice(uint16_t vendorId, uint8_t classId, uint8_t subclass);