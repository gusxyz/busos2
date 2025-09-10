#pragma once
#include <stdint.h>

typedef struct
{
    char Signature[8];
    uint8_t Checksum;
    char OEMID[6];
    uint8_t Revision;
    uint32_t RsdtAddress;
} __attribute__((packed)) RSDP_t;

typedef struct
{

    char Signature[8];
    uint8_t Checksum;
    char OEMID[6];
    uint8_t Revision;
    uint32_t RsdtAddress;

    // Fields specific to the XSDP
    uint32_t Length;
    uint64_t XsdtAddress;
    uint8_t ExtendedChecksum;
    uint8_t reserved[3];
} __attribute__((packed)) XSDP_t;

typedef struct ACPISDTHeader
{
    char Signature[4];
    uint32_t Length;
    uint8_t Revision;
    uint8_t Checksum;
    char OEMID[6];
    char OEMTableID[8];
    uint32_t OEMRevision;
    uint32_t CreatorID;
    uint32_t CreatorRevision;
} ACPISDTHeader_t;

RSDP_t *scan_for_rsdp(uintptr_t start_addr, uintptr_t end_addr);
RSDP_t *find_rsdp(uint32_t ptr);
void acpi_init(uint32_t ptr);
void rsdt_parse();
