#include <rsdp.h>
#include <util.h>
#include <stdio.h>
#include <memory.h>

XSDP_t *g_xsdp = NULL;
RSDP_t *g_rsdp = NULL;

int do_checksum(uint8_t *start_addr, int len)
{
    uint8_t sum = 0;
    for (int i = 0; i < len; i++)
    {
        sum += start_addr[i];
    }
    return sum == 0; // If the lowest byte is 0, the sum is valid
}

RSDP_t *scan_for_rsdp(uintptr_t start, uintptr_t end)
{
    for (uintptr_t ptr = start; ptr < end; ptr += 16)
    {
        if (memcmp((void *)ptr, "RSD PTR ", 8) == 0)
        {
            return (RSDP_t *)ptr;
        }
    }
    return NULL;
}

RSDP_t *find_rsdp(uint32_t ptr)
{
    // 1. Search the EBDA (using the globally stored physical address)
    if (ptr != 0)
    {
        // g_ebda_addr is now a valid virtual address because of the identity map
        RSDP_t *rsdp = scan_for_rsdp(ptr, ptr + 1024);
        if (rsdp != NULL)
        {
            return rsdp;
        }
    }

    // 2. If not in EBDA, search the main BIOS area
    // 0xE0000 is also a valid virtual address now.
    RSDP_t *rsdp = scan_for_rsdp(0x000E0000, 0x000FFFFF);
    if (rsdp != NULL)
    {
        return rsdp;
    }

    return NULL;
}

void acpiInit(uint32_t ptr)
{
    RSDP_t *rsdp_base = find_rsdp(ptr);

    if (!rsdp_base)
    {
        printf("ACPI RSDP signature not found!\n");
        return;
    }

    // --- NOW, VALIDATE AND INTERPRET ---

    // --- DEBUG STEP 1: PRINT THE PHYSICAL ADDRESS OF THE RSDP ---
    // Since find_rsdp scans physical memory, rsdp_base is a physical address.
    printf("DEBUG: RSDP signature found at PHYSICAL address: 0x%x\n", (uint32_t)rsdp_base);

    // --- NOW, VALIDATE AND INTERPRET ---

    if (!do_checksum((uint8_t *)rsdp_base, sizeof(RSDP_t)))
    {
        printf("RSDP found, but failed basic checksum. It might be corrupt.\n");
        // Let's continue anyway for debugging, but this is a red flag.
    }

    // --- DEBUG STEP 2: PRINT THE PHYSICAL ADDRESS STORED *INSIDE* THE RSDP ---
    printf("DEBUG: RSDP claims the RSDT is at PHYSICAL address: 0x%x\n", rsdp_base->RsdtAddress);

    printf("RSDP found with valid checksum. OEMID: %s, Revision: %d\n", rsdp_base->OEMID, rsdp_base->Revision);

    // 2. Check the Revision to decide what to do next.
    if (rsdp_base->Revision == 0)
    {
        printf("ACPI 1.0 detected. Using 32-bit RSDT at 0x%x\n", rsdp_base->RsdtAddress);
        g_rsdp = rsdp_base;
    }
    else
    { // Revision is likely 2 or greater
        printf("ACPI 2.0+ detected.\n");

        // Now it's safe to cast to the extended structure
        XSDP_t *xsdp = (XSDP_t *)rsdp_base;

        // The Length field should now be a valid, non-zero number (e.g., 36)
        printf("XSDP Length: %d bytes\n", xsdp->Length);

        if (xsdp->Length == 0)
        {
            printf("ERROR: Revision is > 0 but Length is 0. Corrupt table.\n");
            return;
        }

        // Validate the extended checksum
        if (!do_checksum((uint8_t *)xsdp, xsdp->Length))
        {
            printf("XSDP failed extended checksum.\n");
            return;
        }

        printf("XSDP valid. Using 64-bit XSDT at 0x%llx\n", xsdp->XsdtAddress);
        g_xsdp = xsdp;
    }
}

bool rsdt_validate(ACPISDTHeader_t *tableHeader)
{
    uint8_t sum = 0;

    for (int i = 0; i < tableHeader->Length; i++)
    {
        sum += ((uint8_t *)tableHeader)[i];
    }

    return sum == 0;
}

void rsdt_parse()
{
    if (g_rsdp == NULL)
    {
        printf("RSDP not found or is ACPI 2.0+, cannot parse RSDT.\n");
        return;
    }

    uint32_t rsdt_phys_addr = g_rsdp->RsdtAddress;
    printf("Attempting to parse RSDT at physical address: 0x%x\n", rsdt_phys_addr);

    // --- STAGE 1: CALCULATE PAGE AND OFFSET ---
    uint32_t phys_page_base = rsdt_phys_addr & ~0xFFF; // Address of the page the RSDT starts in
    uint32_t page_offset = rsdt_phys_addr & 0xFFF;     // Offset of the RSDT within that page

    // --- STAGE 2: TEMPORARY MAPPING ---
    // Map just the first page to read the header and get the full length.
    void *mapped_page = vmmAlloc(phys_page_base, 1, PAGE_FLAG_PRESENT);

    if (mapped_page == NULL)
    {
        printf("FATAL: Failed to temporarily map RSDT header.\n");
        return;
    }

    // The header's virtual address is the base of the mapped page + the offset.
    ACPISDTHeader_t *header = (ACPISDTHeader_t *)((uintptr_t)mapped_page + page_offset);

    printf("DEBUG: Physical page 0x%x was mapped to virtual address 0x%x\n", phys_page_base, (uint32_t)mapped_page);
    printf("DEBUG: RSDT header is at virtual address 0x%x\n", (uint32_t)header);

    // --- STAGE 3: SANITY CHECKS ---
    if (memcmp(header->Signature, "RSDT", 4) != 0)
    {
        printf("FATAL: Mapped memory does NOT have a valid 'RSDT' signature.\n");
        vmmUnmapPage((uint32_t)mapped_page); // Clean up the bad mapping
        return;
    }

    uint32_t real_length = header->Length;
    printf("RSDT header reports length: %u\n", real_length);

    if (real_length < sizeof(ACPISDTHeader_t) || real_length > 16384) // 16KB sanity limit
    {
        printf("FATAL: RSDT has a corrupt or unreasonable length.\n");
        vmmUnmapPage((uint32_t)mapped_page); // Clean up
        return;
    }

    // We are done with the temporary one-page mapping.
    vmmUnmapPage((uint32_t)mapped_page);

    // --- STAGE 4: PERMANENT MAPPING ---
    // Now, calculate the total number of pages needed for the *entire* table.
    // This correctly handles tables that cross page boundaries.
    size_t num_pages = (page_offset + real_length + PAGE_SIZE - 1) / PAGE_SIZE;

    // Map the full region
    void *full_mapped_base = vmmAlloc(phys_page_base, num_pages, PAGE_FLAG_PRESENT);
    if (full_mapped_base == NULL)
    {
        printf("FATAL: Failed to map the full RSDT.\n");
        return;
    }

    // The pointer to the full RSDT is again at the offset from the base.
    ACPISDTHeader_t *full_rsdt = (ACPISDTHeader_t *)((uintptr_t)full_mapped_base + page_offset);

    // --- STAGE 5: VALIDATE & PARSE ---
    if (!rsdt_validate(full_rsdt))
    {
        printf("FATAL: Full RSDT checksum is invalid!\n");
        vmmUnmapRegion((uint32_t)full_mapped_base, num_pages); // Clean up
        return;
    }

    printf("RSDT is valid. Parsing other tables...\n");

    int num_pointers = (full_rsdt->Length - sizeof(ACPISDTHeader_t)) / 4;
    uint32_t *pointer_array = (uint32_t *)((void *)full_rsdt + sizeof(ACPISDTHeader_t));

    for (int i = 0; i < num_pointers; i++)
    {
        uint32_t other_table_phys_addr = pointer_array[i];
        printf("Found table at physical address: 0x%x\n", other_table_phys_addr);
        // Now you would repeat this whole map/validate/parse process for this new table...
    }

    // Remember to unmap the full region when you are done with it!
    // vmmUnmapRegion((uint32_t)full_mapped_base, num_pages);
}
