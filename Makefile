# Sysroot DIR
INCLUDE_DIR=include/
# Compiler and flags
CC = /home/gus/opt/cross/bin/i686-elf-gcc
CXX = /home/gus/opt/cross/bin/i686-elf-g++
GDB = ~/opt/cross/bin/i686-elf-gdb os/boot/kernel.bin
ASM = nasm
QEMU = qemu-system-i386
CFLAGS = -m32 -g -ffreestanding -fno-exceptions -nostdlib -Wall -Wextra -I $(INCLUDE_DIR)
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti -std=c++11
ASMFLAGS = -f elf32
LDFLAGS = -T linker.ld -nostdlib

# Directories
SRC_DIR = src
KERNEL_SRC_DIR = $(SRC_DIR)/kernel
BUILD_DIR = build
INTERMEDIATE_DIR = $(BUILD_DIR)/intermediate
KERNEL_OUT_DIR = os/boot
ISO_DIR = $(BUILD_DIR)

# --- AUTOMATIC SOURCE AND OBJECT FILE DISCOVERY ---
C_SOURCES := $(shell find $(KERNEL_SRC_DIR) -name '*.c')
C_SOURCES += $(shell find $(INCLUDE_DIR) -name '*.c')
CXX_SOURCES := $(shell find $(SRC_DIR) -name '*.cpp')
ALL_ASM_SOURCES := $(shell find $(KERNEL_SRC_DIR) -name '*.s')
SFN_SOURCES := $(shell find $(KERNEL_SRC_DIR) -name '*.sfn')

# Separate CRT files from regular assembly files
CRT_ASM_SOURCES := $(filter %crti.s %crtn.s, $(ALL_ASM_SOURCES))
ASM_SOURCES := $(filter-out %crti.s %crtn.s, $(ALL_ASM_SOURCES))

# Generate lists of object file basenames
C_OBJS := $(patsubst %.c,%.o,$(notdir $(C_SOURCES)))
CXX_OBJS := $(patsubst %.cpp,%.o,$(notdir $(CXX_SOURCES)))
ASM_OBJS := $(patsubst %.s,%.o,$(notdir $(ASM_SOURCES)))
SFN_OBJS := $(patsubst %.sfn,%.o,$(notdir $(SFN_SOURCES)))

# Create a list of all your kernel's object files
KERNEL_OBJECTS := $(addprefix $(INTERMEDIATE_DIR)/, $(C_OBJS) $(CXX_OBJS) $(ASM_OBJS) $(SFN_OBJS))

# Define CRT object files provided by the compiler and built by you
CRTI_OBJ := $(INTERMEDIATE_DIR)/crti.o
CRTN_OBJ := $(INTERMEDIATE_DIR)/crtn.o
CRTBEGIN_OBJ := $(shell $(CC) $(CFLAGS) -print-file-name=crtbegin.o)
CRTEND_OBJ := $(shell $(CC) $(CFLAGS) -print-file-name=crtend.o)

# The final, ordered list of all object files for the linker
LINK_LIST := $(CRTI_OBJ) $(CRTBEGIN_OBJ) $(KERNEL_OBJECTS) $(CRTEND_OBJ) $(CRTN_OBJ)

VPATH := $(shell find $(SRC_DIR) $(INCLUDE_DIR) -type d | tr '\n' ':')

# Output files
KERNEL_BIN = $(KERNEL_OUT_DIR)/kernel.bin
ISO_IMAGE = $(ISO_DIR)/bus.iso
DISK_IMAGE = ext2.img

# Default target
all: $(ISO_IMAGE)

# --- GENERIC COMPILATION RULES ---
$(INTERMEDIATE_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(INTERMEDIATE_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(INTERMEDIATE_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS) $< -o $@

$(INTERMEDIATE_DIR)/%.o: %.sfn
	@mkdir -p $(dir $@)
	/home/gus/opt/cross/bin/i686-elf-ld -m elf_i386 -r -b binary -o $@ $<

# Link kernel binary
$(KERNEL_BIN): $(LINK_LIST)
	@mkdir -p $(dir $@)
	$(CXX) -m32 -g $(LDFLAGS) -o $@ -ffreestanding -O2 $(LINK_LIST) -lgcc

$(ISO_IMAGE): $(KERNEL_BIN)
	grub-mkrescue -o $@ -r ./os/

clean:
	rm -rf $(BUILD_DIR) $(KERNEL_BIN)

.PHONY: all clean

run: $(ISO_IMAGE)
	$(QEMU) -cdrom $(ISO_IMAGE) -hda $(DISK_IMAGE) -serial stdio -monitor none

debug: $(ISO_IMAGE)
	$(QEMU) -cdrom $(ISO_IMAGE) -hda $(DISK_IMAGE) -serial stdio -monitor none -s -S

gdb:
	$(GDB)