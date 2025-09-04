# Sysroot DIR
INCLUDE_DIR=sysroot/include
# Compiler and flags
CC = /home/gus/opt/cross/bin/i686-elf-gcc
ASM = nasm
LD = ld
CFLAGS =  -g -ffreestanding -Wall -Wextra -O2 -I $(INCLUDE_DIR)
ASMFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld

# Directories
SRC_DIR = src
KERNEL_SRC_DIR = $(SRC_DIR)/kernel
BUILD_DIR = build
INTERMEDIATE_DIR = $(BUILD_DIR)/intermediate
KERNEL_OUT_DIR = os/boot
ISO_DIR = $(BUILD_DIR)

# --- AUTOMATIC SOURCE AND OBJECT FILE DISCOVERY ---
# 1. Find all source files using 'shell find'. This is robust.
C_SOURCES := $(shell find $(KERNEL_SRC_DIR) -name '*.c')
C_SOURCES += $(shell find $(INCLUDE_DIR) -name '*.c') # Add liballoc.c

ASM_SOURCES := $(shell find $(KERNEL_SRC_DIR) -name '*.s')
SFN_SOURCES := $(shell find $(KERNEL_SRC_DIR) -name '*.sfn')

# 2. Generate the list of object files from the source files.
#    'notdir' gets the filename (e.g., kernel.c), 'patsubst' changes .c to .o.
C_OBJS := $(patsubst %.c,%.o,$(notdir $(C_SOURCES)))
ASM_OBJS := $(patsubst %.s,%.o,$(notdir $(ASM_SOURCES)))
SFN_OBJS := $(patsubst %.sfn,%.o,$(notdir $(SFN_SOURCES)))

# 3. Create the final OBJECTS list by adding the build directory prefix.
OBJECTS := $(addprefix $(INTERMEDIATE_DIR)/, $(C_OBJS) $(ASM_OBJS) $(SFN_OBJS))

# 4. Tell 'make' where to find the source files for the generic rules below.
VPATH := $(shell find $(SRC_DIR) $(INCLUDE_DIR) -type d | tr '\n' ':')

# Output files
KERNEL_BIN = $(KERNEL_OUT_DIR)/kernel.bin
ISO_IMAGE = $(ISO_DIR)/bus.iso
DISK_IMAGE = $(ISO_DIR)/busos.img

# Default target
all: $(ISO_IMAGE)

# --- GENERIC COMPILATION RULES ---
# These rules replace ALL of your old, separate compilation rules.

# Generic rule to compile ANY .c file found via VPATH
$(INTERMEDIATE_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Generic rule to assemble ANY .s file found via VPATH
$(INTERMEDIATE_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS) $< -o $@

# Generic rule for the .sfn font file
$(INTERMEDIATE_DIR)/%.o: %.sfn
	@mkdir -p $(dir $@)
	$(LD) -m elf_i386 -r -b binary -o $@ $<

# Link kernel binary
$(KERNEL_BIN): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS)

# ... (The rest of your Makefile: grub-mkrescue, clean, run, etc. stays the same) ...
	
$(ISO_IMAGE): $(KERNEL_BIN)
	grub-mkrescue -o $@ -r ./os/

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(KERNEL_BIN)

# Phony targets
.PHONY: all clean

run: os/boot/kernel.bin
	qemu-system-i386 -cdrom build/bus.iso -serial stdio -monitor none

debug: os/boot/kernel.bin
	qemu-system-i386 -cdrom build/bus.iso -serial stdio -monitor stdio -s -S