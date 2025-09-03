# Compiler and flags
CC = /home/gus/opt/cross/bin/i686-elf-gcc
ASM = nasm
LD = ld
CFLAGS =  -g -ffreestanding -Wall -Wextra -O2
ASMFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld

# Directories
SRC_DIR = src
KERNEL_DIR = $(SRC_DIR)/kernel
BUILD_DIR = build
INTERMEDIATE_DIR = $(BUILD_DIR)/intermediate
KERNEL_DIR = os/boot
ISO_DIR = $(BUILD_DIR)

# Object files
BOOT_OBJ = $(INTERMEDIATE_DIR)/boot.o
GDT_OBJ = $(INTERMEDIATE_DIR)/gdt.o
KERNEL_OBJ = $(INTERMEDIATE_DIR)/kernel.o
VGA_OBJ = $(INTERMEDIATE_DIR)/vga.o
GDTS_OBJ = $(INTERMEDIATE_DIR)/gdts.o
STDIO_OBJ = $(INTERMEDIATE_DIR)/stdio.o
IDTS_OBJ = $(INTERMEDIATE_DIR)/idts.o
IDT_OBJ = $(INTERMEDIATE_DIR)/idt.o
UTIL_OBJ = $(INTERMEDIATE_DIR)/util.o
TIMER_OBJ = $(INTERMEDIATE_DIR)/timer.oxxz
KEYBOARD_OBJ = $(INTERMEDIATE_DIR)/keyboard.o
MEMORY_OBJ = $(INTERMEDIATE_DIR)/memory.o
LIBALLOC_OBJ = $(INTERMEDIATE_DIR)/liballoc.o
LIBALLOCHOOKS_OBJ = $(INTERMEDIATE_DIR)/liballoc_hooks.o
PCI_OBJ = $(INTERMEDIATE_DIR)/pci.o
IDE_OBJ = $(INTERMEDIATE_DIR)/ide.o
FS_OBJ = $(INTERMEDIATE_DIR)/filesystem.o
FONT_OBJ = $(INTERMEDIATE_DIR)/zap-light18.o
VBE_OBJ = $(INTERMEDIATE_DIR)/vbe.o
VBE_OBJ = $(INTERMEDIATE_DIR)/vbe.o
OBJECTS = $(BOOT_OBJ) $(KERNEL_OBJ) $(STDIO_OBJ) $(VGA_OBJ) $(GDT_OBJ) $(GDTS_OBJ) $(UTIL_OBJ) $(IDT_OBJ) $(IDTS_OBJ) $(TIMER_OBJ) $(KEYBOARD_OBJ) $(MEMORY_OBJ) $(LIBALLOC_OBJ) $(LIBALLOCHOOKS_OBJ) $(PCI_OBJ) $(IDE_OBJ) $(FS_OBJ) $(FONT_OBJ) $(VBE_OBJ)

# Output files
KERNEL_BIN = $(KERNEL_DIR)/kernel.bin
ISO_IMAGE = $(ISO_DIR)/bus.iso
DISK_IMAGE = $(ISO_DIR)/busos.img

# Default target
all: $(ISO_IMAGE)

# Compile stdlib C files
$(INTERMEDIATE_DIR)/%.o: $(SRC_DIR)/kernel/stdlib/%.c
	@mkdir -p $(INTERMEDIATE_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile driver files
$(INTERMEDIATE_DIR)/%.o: $(SRC_DIR)/kernel/drivers/%.c
	@mkdir -p $(INTERMEDIATE_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
	
# Compile fs C files
$(INTERMEDIATE_DIR)/%.o: $(SRC_DIR)/kernel/filesystem/%.c
	@mkdir -p $(INTERMEDIATE_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile gdt C files
$(INTERMEDIATE_DIR)/%.o: $(SRC_DIR)/kernel/gdt/%.c
	@mkdir -p $(INTERMEDIATE_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile idt C files
$(INTERMEDIATE_DIR)/%.o: $(SRC_DIR)/kernel/idt/%.c
	@mkdir -p $(INTERMEDIATE_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile liballoc files
$(INTERMEDIATE_DIR)/%.o: $(SRC_DIR)/kernel/liballoc/%.c
	@mkdir -p $(INTERMEDIATE_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile vesa files
$(INTERMEDIATE_DIR)/%.o: $(SRC_DIR)/kernel/vesa/%.c
	@mkdir -p $(INTERMEDIATE_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile console files
$(INTERMEDIATE_DIR)/%.o: $(SRC_DIR)/kernel/console/%.c
	@mkdir -p $(INTERMEDIATE_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile Kernel C files
$(INTERMEDIATE_DIR)/%.o: $(SRC_DIR)/kernel/%.c
	@mkdir -p $(INTERMEDIATE_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
	
# Assemble boot.s
$(INTERMEDIATE_DIR)/%.o: $(SRC_DIR)/kernel/%.s
	@mkdir -p $(INTERMEDIATE_DIR)
	$(ASM) $(ASMFLAGS) $< -o $@

# Assemble gdt.s files
$(INTERMEDIATE_DIR)/%.o: $(SRC_DIR)/kernel/gdt/%.s
	@mkdir -p $(INTERMEDIATE_DIR)
	$(ASM) $(ASMFLAGS) $< -o $@

# Assemble idt.s files
$(INTERMEDIATE_DIR)/%.o: $(SRC_DIR)/kernel/idt/%.s
	@mkdir -p $(INTERMEDIATE_DIR)
	$(ASM) $(ASMFLAGS) $< -o $@

$(INTERMEDIATE_DIR)/%.o: $(SRC_DIR)fonts/%.psf
	@mkdir -p $(INTERMEDIATE_DIR)
	objcopy -O elf32-i386 -B i386 -I binary $@ $<

$(INTERMEDIATE_DIR)/%.o: $(SRC_DIR)/kernel/fonts/%.psf
	@mkdir -p $(INTERMEDIATE_DIR)
	objcopy -O elf32-i386 -B i386 -I binary $@ $<

# Link kernel binary
$(KERNEL_BIN): $(OBJECTS)
	@mkdir -p $(KERNEL_DIR)
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS)
	
$(ISO_IMAGE): $(KERNEL_BIN)
	grub-mkrescue -o $@ -r ./os/

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(KERNEL_BIN)

# Phony targets
.PHONY: all clean

