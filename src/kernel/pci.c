#include "pci.h"
#include "util.h"
#include "stdlib/stdio.h"
#include <stddef.h>

static pci_device_t pciDevices[MAX_PCI_DEVICES];
static int pciDeviceCount = 0;

void pciInit(bool dumpPCI)
{
    checkAllBuses();
    printf("PCI Initialized\n");
    if (!dumpPCI)
        return;
    for (int i = 0; i < MAX_PCI_DEVICES; i++)
    {
        pci_device_t pciDevice = pciDevices[i];
        if (pciDevice.vendor_id == 0x0)
            continue;
        printf("PCI DEVICE: Vendor Id: 0x%x Device Id: 0x%x Class: 0x%x Subclass: 0x%x\n Header Type:0x%x\n", pciDevice.vendor_id, pciDevice.device_id, pciDevice.class_code, pciDevice.subclass, pciDevice.header_type);
    }
}
uint32_t pciConfigReadDWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
    uint32_t address;
    uint32_t lbus = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;

    // Create the 32-bit address for the PCI configuration space.
    // The (offset & 0xFC) part ensures we are aligned to a 4-byte boundary.
    address = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xFC) | 0x80000000);

    // Write the address to the Address Port (0xCF8).
    outl(0xCF8, address);

    // Read the full 32-bit DWord from the Data Port (0xCFC) and return it.
    return inl(0xCFC);
}

uint16_t pciConfigReadWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
    uint32_t dWord = pciConfigReadDWord(bus, slot, func, offset);

    // (offset & 2) * 8) = 0 will choose the first word of the 32-bit register
    return (uint16_t)((dWord >> ((offset & 2) * 8)) & 0xFFFF);
    ;
}

pci_device_t *pciFindDevice(uint16_t vendorId, uint8_t classId, uint8_t subclass)
{
    for (int i = 0; i < MAX_PCI_DEVICES; i++)
    {
        if (pciDevices[i].vendor_id == vendorId &&
            pciDevices[i].class_code == classId &&
            pciDevices[i].subclass == subclass)
        {
            return &pciDevices[i];
        }
    }

    return NULL;
}

uint16_t pciGetVendorID(uint8_t bus, uint8_t device, uint8_t func)
{
    return pciConfigReadWord(bus, device, func, 0);
}

uint16_t pciGetDeviceID(uint8_t bus, uint8_t device, uint8_t func)
{
    return pciConfigReadWord(bus, device, func, 2);
}

uint8_t pciGetProgIF(uint8_t bus, uint8_t device, uint8_t func)
{
    // Read the 16-bit word at offset 0x8. This contains Prog IF | Revision ID.
    uint16_t class_subclass = pciConfigReadWord(bus, device, func, 0x8);

    // The Prog IF is the high byte, so shift right by 8.
    return (uint8_t)(class_subclass >> 8);
}

// Reads the Class Code of a PCI device.
uint8_t pciGetClass(uint8_t bus, uint8_t device, uint8_t func)
{
    // Read the 16-bit word at offset 0xA. This contains Class Code | Subclass.
    uint16_t class_subclass = pciConfigReadWord(bus, device, func, 0xA);

    // The Class Code is the high byte, so shift right by 8.
    return (uint8_t)(class_subclass >> 8);
}

// Reads the Subclass of a PCI device.
uint8_t pciGetSubclass(uint8_t bus, uint8_t device, uint8_t func)
{
    // Read the 16-bit word at offset 0xA.
    uint16_t class_subclass = pciConfigReadWord(bus, device, func, 0xA);

    // The Subclass is the low byte, so we can just cast or mask.
    return (uint8_t)(class_subclass & 0xFF);
}

uint8_t pciGetHeaderType(uint8_t bus, uint8_t device, uint8_t func)
{
    // Read the 16-bit word at offset 0xE.
    uint16_t bist_header = pciConfigReadWord(bus, device, func, 0xE);

    // The Header is the low byte, so we can just cast or mask.
    return (uint8_t)(bist_header & 0xFF);
}

void checkDevice(uint8_t bus, uint8_t device)
{
    uint8_t function = 0;

    int vendorID = pciGetVendorID(bus, device, function);
    if (vendorID == 0xFFFF)
        return; // Device doesn't exist
    checkFunction(bus, device, function);

    int headerType = pciGetHeaderType(bus, device, function);
    if ((headerType & 0x80) != 0)
    {
        // It's a multi-function device, so check remaining functions
        for (function = 1; function < 8; function++)
        {
            if (pciGetVendorID(bus, device, function) != 0xFFFF)
            {
                checkFunction(bus, device, function);
            }
        }
    }
}

void checkAllBuses()
{
    for (uint16_t bus = 0; bus < 256; bus++)
    {
        for (uint8_t device = 0; device < 32; device++)
        {
            checkDevice(bus, device);
        }
    }
}

void checkFunction(uint8_t bus, uint8_t slot, uint8_t func)
{
    if (pciDeviceCount >= MAX_PCI_DEVICES)
    {
        printf("PCI: Warning! Found more than MAX_PCI_DEVICES. Ignoring rest.\n");
        return; // Stop scanning
    }

    // 2. Get a pointer to the next available slot
    pci_device_t *device = &pciDevices[pciDeviceCount];

    // 3. Fill the struct with device information
    device->bus = bus;
    device->slot = slot;
    device->func = func;
    device->vendor_id = pciGetVendorID(bus, slot, func);
    device->device_id = pciGetDeviceID(bus, slot, func);
    device->class_code = pciGetClass(bus, slot, func);
    device->subclass = pciGetSubclass(bus, slot, func);
    device->header_type = pciGetHeaderType(bus, slot, func);
    device->prog_if = pciGetProgIF(bus, slot, func);

    if (device->header_type == 0x0)
    {
        device->bar0 = pciConfigReadDWord(bus, slot, func, 0x10);
        device->bar1 = pciConfigReadDWord(bus, slot, func, 0x14);
        device->bar2 = pciConfigReadDWord(bus, slot, func, 0x18);
        device->bar3 = pciConfigReadDWord(bus, slot, func, 0x1C);
        device->bar4 = pciConfigReadDWord(bus, slot, func, 0x20);
        device->bar5 = pciConfigReadDWord(bus, slot, func, 0x24);
    }

    // 4. Increment the counter for the next device
    pciDeviceCount++;
}