#include "ide.h"
#include <stddef.h>
#include "util.h"
#include "timer.h"

ide_channel_register_t channels[2];
ide_device_t ideDevices[4];
uint8_t ideBuf[2048] = {0};
volatile unsigned static char ideIrqInvoked = 0;
unsigned static char atapiPacket[12] = {0xA8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void initIDEController()
{
    pci_device_t *ide_controller = pciFindDevice(0x8086, 0x01, 0x01);
    if (ide_controller == NULL)
    {
        printf("No IDE Controller Found.\n");
        return;
    }
    printf("IDE Controller Found.\n");
    idePrintProg(ide_controller);
    // currently were just only gonna support compatibility mode
    ideInitialize(0x1F0, 0x3F6, 0x170, 0x376, 0x000);
}

void idePrintProg(pci_device_t *device)
{
    // First, confirm this is actually an IDE controller.
    // Class 0x01 = Mass Storage, Subclass 0x01 = IDE.
    if (device->class_code != 0x01 || device->subclass != 0x01)
    {
        // Not an IDE controller, so we have nothing to print.
        return;
    }

    printf("> IDE Controller Details (Prog IF: 0x%x):\n", device->prog_if);

    // --- Primary Channel ---
    if (device->prog_if & 0x01)
    {
        printf("- Primary Channel: PCI Native Mode\n");
    }
    else
    {
        printf("- Primary Channel: Compatibility Mode (Ports 0x1F0-7, 0x3F6, IRQ14)\n");
    }
    // Check if the mode is switchable
    if (device->prog_if & 0x02)
    {
        printf("- Mode is switchable.\n");
    }
    else
    {
        printf("- Mode is fixed.\n");
    }

    // --- Secondary Channel ---
    if (device->prog_if & 0x04)
    {
        printf("- Secondary Channel: PCI Native Mode\n");
    }
    else
    {
        printf("- Secondary Channel: Compatibility Mode (Ports 0x170-7, 0x376, IRQ15)\n");
    }
    // Check if the mode is switchable
    if (device->prog_if & 0x08)
    {
        printf("- Mode is switchable.\n");
    }
    else
    {
        printf("- Mode is fixed.\n");
    }

    // --- Bus Mastering (DMA) ---
    if (device->prog_if & 0x80)
    {
        printf(" Bus Master IDE: Supported (DMA is available)\n");
    }
    else
    {
        printf(" Bus Master IDE: Not supported (PIO only)\n");
    }
}

void ideInitialize(unsigned int BAR0, unsigned int BAR1, unsigned int BAR2, unsigned int BAR3, unsigned int BAR4)
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

    // 3- Detect ATA-ATAPI Devices:
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
                // Device uses 48-Bit Addressing:
                ideDevices[count].Size = *((unsigned int *)(ideBuf + ATA_IDENT_MAX_LBA_EXT));
            else
                // Device uses CHS or 28-bit Addressing:
                ideDevices[count].Size = *((unsigned int *)(ideBuf + ATA_IDENT_MAX_LBA));

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
            printf("Found %s Drive %dGB - %s\n",
                   (const char *[]){"ATA", "ATAPI"}[ideDevices[i].Type], /* Type */
                   ideDevices[i].Size / 1024 / 1024 / 2,                 /* Size */
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

    // --- Part 1: C logic to determine the correct port (no changes here) ---
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