#include <drivers/ide.h>
#include <stddef.h>
#include <util.h>
#include <timer.h>
#include <idt.h>
#include <liballoc.h>
#include <stdio.h>

ide_channel_register_t channels[2];
ide_device_t ideDevices[4];
uint8_t ideBuf[2048] = {0};
volatile unsigned static char ideIrqInvoked = 0;
unsigned static char atapiPacket[12] = {0xA8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void init_ide()
{
    pci_device_t *ide_controller = pciFindDevice(0x8086, 0x01, 0x01);
    if (ide_controller == NULL)
    {
        // printf("No IDE Controller Found.\n");
        return;
    }
    // printf("IDE Controller Found.\n");
    // idePrintProg(ide_controller);

    // currently were just gonna support compatibility mode
    // to make this more robust check prog if for pci then pass the BARs from the device
    init_controller(0x1F0, 0x3F6, 0x170, 0x376, 0x000);
    irq_install_handler(14, &ide_irq_handle);
    irq_install_handler(15, &ide_irq_handle);
    return;
}

void ide_irq_handle(struct InterruptRegisters *r)
{
    ideIrqInvoked = 1;
}

void init_controller(unsigned int BAR0, unsigned int BAR1, unsigned int BAR2, unsigned int BAR3, unsigned int BAR4)
{
    int j, k, count = 0;

    // 1- Detect I/O Ports which interface IDE Controller:
    channels[ATA_PRIMARY].base = (BAR0 & 0xFFFFFFFC) + 0x1F0 * (!BAR0);
    channels[ATA_PRIMARY].ctrl = (BAR1 & 0xFFFFFFFC) + 0x3F6 * (!BAR1);
    channels[ATA_SECONDARY].base = (BAR2 & 0xFFFFFFFC) + 0x170 * (!BAR2);
    channels[ATA_SECONDARY].ctrl = (BAR3 & 0xFFFFFFFC) + 0x376 * (!BAR3);
    channels[ATA_PRIMARY].bmide = (BAR4 & 0xFFFFFFFC) + 0;   // Bus Master IDE
    channels[ATA_SECONDARY].bmide = (BAR4 & 0xFFFFFFFC) + 8; // Bus Master IDE

    // 2 - Disable IRQs
    ideWrite(ATA_PRIMARY, ATA_REG_CONTROL, 2);
    ideWrite(ATA_SECONDARY, ATA_REG_CONTROL, 2);

    // 3 - Detect ATA-ATAPI Devices:
    for (int i = 0; i < 2; i++)
    {
        for (j = 0; j < 2; j++)
        {
            uint8_t err = 0, type = IDE_ATA, status;
            ideDevices[count].Reserved = 0; // Assuming that no drive here.

            // (I) Select Drive:
            ideWrite(i, ATA_REG_HDDEVSEL, 0xA0 | (j << 4)); // Select Drive.
            sleep(1);                                       // Wait 1ms for drive select to work.

            // (II) Send ATA Identify Command:
            ideWrite(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
            sleep(1); // This function should be implemented in your OS. which waits for 1 ms.
                      // it is based on System Timer Device Driver.

            // (III) Polling:
            if (ideRead(i, ATA_REG_STATUS) == 0)
                continue; // If Status = 0, No Device.

            while (1)
            {
                status = ideRead(i, ATA_REG_STATUS);
                if ((status & ATA_SR_ERR))
                {
                    err = 1;
                    break;
                } // If Err, Device is not ATA.
                if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ))
                    break; // Everything is right.
            }

            // (IV) Probe for ATAPI Devices:

            if (err != 0)
            {
                uint8_t cl = ideRead(i, ATA_REG_LBA1);
                uint8_t ch = ideRead(i, ATA_REG_LBA2);

                if (cl == 0x14 && ch == 0xEB)
                    type = IDE_ATAPI;
                else if (cl == 0x69 && ch == 0x96)
                    type = IDE_ATAPI;
                else
                    continue; // Unknown Type (may not be a device).

                ideWrite(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
                sleep(1);
            }

            // (V) Read Identification Space of the Device:
            ideReadBuffer(i, ATA_REG_DATA, (unsigned int)ideBuf, 128);

            // (VI) Read Device Parameters:
            ideDevices[count].Reserved = 1;
            ideDevices[count].Type = type;
            ideDevices[count].Channel = i;
            ideDevices[count].Drive = j;
            ideDevices[count].Signature = *((uint16_t *)(ideBuf + ATA_IDENT_DEVICETYPE));
            ideDevices[count].Capabilities = *((uint16_t *)(ideBuf + ATA_IDENT_CAPABILITIES));
            ideDevices[count].CommandSets = *((unsigned int *)(ideBuf + ATA_IDENT_COMMANDSETS));

            // (VII) Get Size:
            if (ideDevices[count].CommandSets & (1 << 26))
                ideDevices[count].Size = *((unsigned int *)(ideBuf + ATA_IDENT_MAX_LBA_EXT)); // Device uses 48-Bit Addressing:
            else
                ideDevices[count].Size = *((unsigned int *)(ideBuf + ATA_IDENT_MAX_LBA)); // Device uses CHS or 28-bit Addressing:

            // (VIII) String indicates model of device (like Western Digital HDD and SONY DVD-RW...):
            for (k = 0; k < 40; k += 2)
            {
                ideDevices[count].Model[k] = ideBuf[ATA_IDENT_MODEL + k + 1];
                ideDevices[count].Model[k + 1] = ideBuf[ATA_IDENT_MODEL + k];
            }
            ideDevices[count].Model[40] = 0; // Terminate String.
            count++;
        }
    }

    // 4- Print Summary:
    for (int i = 0; i < 4; i++)
    {
        if (ideDevices[i].Reserved == 1)
        {
            printf("Found %s Drive #%d %is - %s\n",
                   (const char *[]){"ATA", "ATAPI"}[ideDevices[i].Type],
                   i,
                   ideDevices[i].Size,
                   ideDevices[i].Model);
        }
    }
}

uint8_t ideRead(uint8_t channel, uint8_t reg)
{
    uint8_t result;
    if (reg > 0x07 && reg < 0x0C)
        ideWrite(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
    if (reg < 0x08)
        result = inb(channels[channel].base + reg - 0x00);
    else if (reg < 0x0C)
        result = inb(channels[channel].base + reg - 0x06);
    else if (reg < 0x0E)
        result = inb(channels[channel].ctrl + reg - 0x0A);
    else if (reg < 0x16)
        result = inb(channels[channel].bmide + reg - 0x0E);

    if (reg > 0x07 && reg < 0x0C)
        ideWrite(channel, ATA_REG_CONTROL, channels[channel].nIEN);
    return result;
}

void ideWrite(uint8_t channel, uint8_t reg, uint8_t data)
{
    if (reg > 0x07 && reg < 0x0C)
        ideWrite(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
    if (reg < 0x08)
        outb(channels[channel].base + reg - 0x00, data);
    else if (reg < 0x0C)
        outb(channels[channel].base + reg - 0x06, data);
    else if (reg < 0x0E)
        outb(channels[channel].ctrl + reg - 0x0A, data);
    else if (reg < 0x16)
        outb(channels[channel].bmide + reg - 0x0E, data);
    if (reg > 0x07 && reg < 0x0C)
        ideWrite(channel, ATA_REG_CONTROL, channels[channel].nIEN);
}

uint8_t idePrintError(unsigned int drive, uint8_t err)
{
    if (err == 0)
        return err;

    printf("IDE:");
    if (err == 1)
    {
        printf("- Device Fault\n     ");
        err = 19;
    }
    else if (err == 2)
    {
        uint8_t st = ideRead(ideDevices[drive].Channel, ATA_REG_ERROR);
        if (st & ATA_ER_AMNF)
        {
            printf("- No Address Mark Found\n     ");
            err = 7;
        }
        if (st & ATA_ER_TK0NF)
        {
            printf("- No Media or Media Error\n     ");
            err = 3;
        }
        if (st & ATA_ER_ABRT)
        {
            printf("- Command Aborted\n     ");
            err = 20;
        }
        if (st & ATA_ER_MCR)
        {
            printf("- No Media or Media Error\n     ");
            err = 3;
        }
        if (st & ATA_ER_IDNF)
        {
            printf("- ID mark not Found\n     ");
            err = 21;
        }
        if (st & ATA_ER_MC)
        {
            printf("- No Media or Media Error\n     ");
            err = 3;
        }
        if (st & ATA_ER_UNC)
        {
            printf("- Uncorrectable Data Error\n     ");
            err = 22;
        }
        if (st & ATA_ER_BBK)
        {
            printf("- Bad Sectors\n     ");
            err = 13;
        }
    }
    else if (err == 3)
    {
        printf("- Reads Nothing\n     ");
        err = 23;
    }
    else if (err == 4)
    {
        printf("- Write Protected\n     ");
        err = 8;
    }
    printf("- [%s %s] %s\n",
           (const char *[]){"Primary", "Secondary"}[ideDevices[drive].Channel], // Use the channel as an index into the array
           (const char *[]){"Master", "Slave"}[ideDevices[drive].Drive],        // Same as above, using the drive
           ideDevices[drive].Model);

    return err;
}

void ideReadBuffer(uint8_t channel, uint8_t reg, unsigned int buffer, unsigned int quads)
{
    uint16_t port;

    if (reg > 0x07 && reg < 0x0C)
        ideWrite(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);

    if (reg < 0x08)
        port = channels[channel].base + reg - 0x00;
    else if (reg < 0x0C)
        port = channels[channel].base + reg - 0x06;
    else if (reg < 0x0E)
        port = channels[channel].ctrl + reg - 0x0A;
    else
        port = channels[channel].bmide + reg - 0x0E;

    insl_rep(port, ((void *)buffer), quads);
    if (reg > 0x07 && reg < 0x0C)
        ideWrite(channel, ATA_REG_CONTROL, channels[channel].nIEN);
}

uint8_t idePolling(uint8_t channel, unsigned int advanced_check)
{
    for (int i = 0; i < 4; i++)
    {
        ideRead(channel, ATA_REG_ALTSTATUS);
    }
    while (ideRead(channel, ATA_REG_STATUS) & ATA_SR_BSY)
    {
        ;
    }
    if (advanced_check)
    {
        uint8_t state = ideRead(channel, ATA_REG_STATUS); // Read Status Register.
        // (III) Check For Errors
        // -------------------------------------------------
        if (state & ATA_SR_ERR)
            return 2; // Error.

        // (IV) Check If Device fault:
        // -------------------------------------------------
        if (state & ATA_SR_DF)
            return 1; // Device Fault.

        // (V) Check DRQ:
        // -------------------------------------------------
        // BSY = 0; DF = 0; ERR = 0 so we should check for DRQ now.
        if ((state & ATA_SR_DRQ) == 0)
            return 3; // DRQ should be set
    }

    return 0; // No Error.
}

void ideWaitIrq()
{
    while (!ideIrqInvoked)
        ;
    ideIrqInvoked = 0;
}

void ideIrq()
{
    ideIrqInvoked = 1;
}

// TODO: fix determineAdressing
/*
void determineAddressing(unsigned int lba, uint16_t capabilities, uint8_t *lba_mode, uint8_t *lba_io[6], uint8_t *head, uint8_t *sect, uint32_t *cyl)
{
    if (lba >= 0x10000000)
    {
        // LBA48:
        *lba_mode = 2;
        *lba_io[0] = (lba & 0x000000FF) >> 0;
        *lba_io[1] = (lba & 0x0000FF00) >> 8;
        *lba_io[2] = (lba & 0x00FF0000) >> 16;
        *lba_io[3] = (lba & 0xFF000000) >> 24;
        *lba_io[4] = 0; // LBA28 is integer, so 32-bits are enough to access 2TB.
        *lba_io[5] = 0; // LBA28 is integer, so 32-bits are enough to access 2TB.
        *head = 0;      // Lower 4-bits of HDDEVSEL are not used here.
    }
    else if (capabilities & 0x200)
    {
        // LBA28:
        *lba_mode = 1;
        *lba_io[0] = (lba & 0x00000FF) >> 0;
        *lba_io[1] = (lba & 0x000FF00) >> 8;
        *lba_io[2] = (lba & 0x0FF0000) >> 16;
        *lba_io[3] = 0; // These Registers are not used here.
        *lba_io[4] = 0; // These Registers are not used here.
        *lba_io[5] = 0; // These Registers are not used here.
        *head = (lba & 0xF000000) >> 24;
    }
    else
    {
        // CHS:
        *lba_mode = 0;
        *sect = (lba % 63) + 1;
        *cyl = (lba + 1 - *sect) / (16 * 63);
        uint8_t cylVal = *cyl;
        lba_io[0] = *sect;
        lba_io[1] = (cylVal >> 0) & 0xFF;
        lba_io[2] = (cylVal >> 8) & 0xFF;
        lba_io[3] = 0;
        lba_io[4] = 0;
        lba_io[5] = 0;
        *head = (lba + 1 - *sect) % (16 * 63) / (63);
    }
}
*/

uint8_t ide_ata_rw(uint8_t direction, uint8_t drive, unsigned int lba, uint8_t numsects, uint32_t selector, unsigned int edi)
{
    uint8_t lba_mode; /* 0: CHS, 1:LBA28, 2: LBA48 */
    uint8_t dma;      /* 0: No DMA, 1: DMA */
    uint8_t cmd;
    uint8_t lba_io[6];
    unsigned int channel = ideDevices[drive].Channel; // Read the Channel.
    unsigned int slavebit = ideDevices[drive].Drive;  // Read the Drive [Master/Slave]
    unsigned int bus = channels[channel].base;        // Bus Base, like 0x1F0 which is also data port.
    unsigned int words = 256;                         // Almost every ATA drive has a sector-size of 512-byte.
    uint32_t cyl, i;
    uint8_t head, sect, err;

    ideWrite(channel, ATA_REG_CONTROL, channels[channel].nIEN = (ideIrqInvoked = 0x0) + 0x02);

    if (lba >= 0x10000000)
    { // Sure Drive should support LBA in this case, or you are
      // giving a wrong LBA.
        // LBA48:
        lba_mode = 2;
        lba_io[0] = (lba & 0x000000FF) >> 0;
        lba_io[1] = (lba & 0x0000FF00) >> 8;
        lba_io[2] = (lba & 0x00FF0000) >> 16;
        lba_io[3] = (lba & 0xFF000000) >> 24;
        lba_io[4] = 0; // LBA28 is integer, so 32-bits are enough to access 2TB.
        lba_io[5] = 0; // LBA28 is integer, so 32-bits are enough to access 2TB.
        head = 0;      // Lower 4-bits of HDDEVSEL are not used here.
    }
    else if (ideDevices[drive].Capabilities & 0x200)
    { // Drive supports LBA?
        // LBA28:
        lba_mode = 1;
        lba_io[0] = (lba & 0x00000FF) >> 0;
        lba_io[1] = (lba & 0x000FF00) >> 8;
        lba_io[2] = (lba & 0x0FF0000) >> 16;
        lba_io[3] = 0; // These Registers are not used here.
        lba_io[4] = 0; // These Registers are not used here.
        lba_io[5] = 0; // These Registers are not used here.
        head = (lba & 0xF000000) >> 24;
    }
    else
    {
        // CHS:
        lba_mode = 0;
        sect = (lba % 63) + 1;
        cyl = (lba + 1 - sect) / (16 * 63);
        lba_io[0] = sect;
        lba_io[1] = (cyl >> 0) & 0xFF;
        lba_io[2] = (cyl >> 8) & 0xFF;
        lba_io[3] = 0;
        lba_io[4] = 0;
        lba_io[5] = 0;
        head = (lba + 1 - sect) % (16 * 63) / (63); // Head number is written to HDDEVSEL lower 4-bits.
    }

    dma = 0; // We don't support DMA.. yet

    while (ideRead(channel, ATA_REG_STATUS) & ATA_SR_BSY)
    {
        ;
    }

    if (lba_mode == 0)
        ideWrite(channel, ATA_REG_HDDEVSEL, 0xA0 | (slavebit << 4) | head); // Drive & CHS.
    else
        ideWrite(channel, ATA_REG_HDDEVSEL, 0xE0 | (slavebit << 4) | head); // Drive & LBA

    if (lba_mode == 2)
    {
        ideWrite(channel, ATA_REG_SECCOUNT1, 0);
        ideWrite(channel, ATA_REG_LBA3, lba_io[3]);
        ideWrite(channel, ATA_REG_LBA4, lba_io[4]);
        ideWrite(channel, ATA_REG_LBA5, lba_io[5]);
    }

    ideWrite(channel, ATA_REG_SECCOUNT0, numsects);
    ideWrite(channel, ATA_REG_LBA0, lba_io[0]);
    ideWrite(channel, ATA_REG_LBA1, lba_io[1]);
    ideWrite(channel, ATA_REG_LBA2, lba_io[2]);

    if (lba_mode == 0 && dma == 0 && direction == 0)
        cmd = ATA_CMD_READ_PIO;
    if (lba_mode == 1 && dma == 0 && direction == 0)
        cmd = ATA_CMD_READ_PIO;
    if (lba_mode == 2 && dma == 0 && direction == 0)
        cmd = ATA_CMD_READ_PIO_EXT;
    if (lba_mode == 0 && dma == 1 && direction == 0)
        cmd = ATA_CMD_READ_DMA;
    if (lba_mode == 1 && dma == 1 && direction == 0)
        cmd = ATA_CMD_READ_DMA;
    if (lba_mode == 2 && dma == 1 && direction == 0)
        cmd = ATA_CMD_READ_DMA_EXT;
    if (lba_mode == 0 && dma == 0 && direction == 1)
        cmd = ATA_CMD_WRITE_PIO;
    if (lba_mode == 1 && dma == 0 && direction == 1)
        cmd = ATA_CMD_WRITE_PIO;
    if (lba_mode == 2 && dma == 0 && direction == 1)
        cmd = ATA_CMD_WRITE_PIO_EXT;
    if (lba_mode == 0 && dma == 1 && direction == 1)
        cmd = ATA_CMD_WRITE_DMA;
    if (lba_mode == 1 && dma == 1 && direction == 1)
        cmd = ATA_CMD_WRITE_DMA;
    if (lba_mode == 2 && dma == 1 && direction == 1)
        cmd = ATA_CMD_WRITE_DMA_EXT;
    ideWrite(channel, ATA_REG_COMMAND, cmd);

    // implement dma smh
    if (dma)
        if (direction == 0)
            ;
        // DMA Read.
        else
            ;
    // DMA Write.
    else if (direction == 0)
        // PIO Read.
        for (i = 0; i < numsects; i++)
        {
            if (err = idePolling(channel, 1))
                return err;

            ataReadSector(selector, (void *)edi, bus, words);
            edi += (words * 2);
        }
    else
    {

        for (i = 0; i < numsects; i++)
        {
            idePolling(channel, 0); // Polling.
            asm volatile(
                "pushl %%ds\n\t"
                "movw %w[sel], %%ds\n\t" // %w for 16-bit selector
                "rep outsw\n\t"
                "popl %%ds\n\t"
                : "+S"(edi), "+c"(words)
                : "d"(bus), [sel] "r"(selector)
                : "memory");
            edi += (words * 2);
        }
        ideWrite(channel, ATA_REG_COMMAND, (char[]){ATA_CMD_CACHE_FLUSH, ATA_CMD_CACHE_FLUSH, ATA_CMD_CACHE_FLUSH_EXT}[lba_mode]);
        idePolling(channel, 0); // Polling.
    }
    ideWrite(channel, ATA_REG_COMMAND, (char[]){ATA_CMD_CACHE_FLUSH, ATA_CMD_CACHE_FLUSH, ATA_CMD_CACHE_FLUSH_EXT}[lba_mode]);
    idePolling(channel, 0);

    return 0;
}

uint8_t ide_atapi_read(uint8_t drive, unsigned int lba, uint8_t numsects, unsigned short selector, unsigned int edi)
{
    unsigned int channel = ideDevices[drive].Channel;
    unsigned int slavebit = ideDevices[drive].Drive;
    unsigned int bus = channels[channel].base;
    unsigned int words = 1024; // Sector Size. ATAPI drives have a sector size of 2048 bytes.
    uint8_t err;
    int i;

    ideWrite(channel, ATA_REG_CONTROL, channels[channel].nIEN = ideIrqInvoked = 0x0);

    atapiPacket[0] = ATAPI_CMD_READ;
    atapiPacket[1] = 0x0;
    atapiPacket[2] = (lba >> 24) & 0xFF;
    atapiPacket[3] = (lba >> 16) & 0xFF;
    atapiPacket[4] = (lba >> 8) & 0xFF;
    atapiPacket[5] = (lba >> 0) & 0xFF;
    atapiPacket[6] = 0x0;
    atapiPacket[7] = 0x0;
    atapiPacket[8] = 0x0;
    atapiPacket[9] = numsects;
    atapiPacket[10] = 0x0;
    atapiPacket[11] = 0x0;

    ideWrite(channel, ATA_REG_HDDEVSEL, slavebit << 4);

    // 400 nanosecond delay for drive select to finish
    for (int i = 0; i < 4; i++)
        ideRead(channel, ATA_REG_ALTSTATUS);

    ideWrite(channel, ATA_REG_FEATURES, 0);

    ideWrite(channel, ATA_REG_LBA1, (words * 2) & 0xFF); // Lower Byte of Sector Size.
    ideWrite(channel, ATA_REG_LBA2, (words * 2) >> 8);   // Upper Byte of Sector Size.

    // (VI): Send the Packet Command:
    // ------------------------------------------------------------------
    ideWrite(channel, ATA_REG_COMMAND, ATA_CMD_PACKET); // Send the Command.

    // (VII): Waiting for the driver to finish or return an error code:
    // ------------------------------------------------------------------
    if (err = idePolling(channel, 1))
        return err; // Polling and return if error.

    // (VIII): Sending the packet data:
    // ------------------------------------------------------------------
    asm volatile("rep   outsw" : : "c"(6), "d"(bus), "S"(atapiPacket)); // Send Packet Data
    for (i = 0; i < numsects; i++)
    {
        ideWaitIrq(); // Wait for an IRQ.
        if ((err = idePolling(channel, 1)))
        {
            printf("ATAPI: Error polling after IRQ for sector %d\n", i);
            return err; // Polling failed, abort.
        }
        ataReadSector(selector, (void *)edi, bus, words);
        edi += (words * 2);
    }

    ideWaitIrq();

    // (XI): Waiting for BSY & DRQ to clear:
    // ------------------------------------------------------------------
    while (ideRead(channel, ATA_REG_STATUS) & (ATA_SR_BSY | ATA_SR_DRQ))
        ;

    return 0; // Easy, ... Isn't it?
}

void idePrintProg(pci_device_t *device)
{

    if (device->class_code != 0x01 || device->subclass != 0x01)
    {
        // Not an IDE controller
        return;
    }

    printf("> IDE Controller Details (Prog IF: 0x%x):\n", device->prog_if);

    if (device->prog_if & 0x01)
    {
        printf("- Primary Channel: PCI Native Mode\n");
    }
    else
    {
        printf("- Primary Channel: Compatibility Mode (Ports 0x1F0-7, 0x3F6, IRQ14)\n");
    }
    if (device->prog_if & 0x02)
    {
        printf("- Mode is switchable.\n");
    }
    else
    {
        printf("- Mode is fixed.\n");
    }

    if (device->prog_if & 0x04)
    {
        printf("- Secondary Channel: PCI Native Mode\n");
    }
    else
    {
        printf("- Secondary Channel: Compatibility Mode (Ports 0x170-7, 0x376, IRQ15)\n");
    }

    if (device->prog_if & 0x08)
    {
        printf("- Mode is switchable.\n");
    }
    else
    {
        printf("- Mode is fixed.\n");
    }

    if (device->prog_if & 0x80)
    {
        printf(" Bus Master IDE: Supported (DMA is available)\n");
    }
    else
    {
        printf(" Bus Master IDE: Not supported (PIO only)\n");
    }
}
