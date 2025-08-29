#include <stdint.h>

struct multiboot_aout_symbol_table
{
    uint32_t tabsize;
    uint32_t strsize;
    uint32_t addr;
    uint32_t reserved;
};

struct multiboot_elf_section_header_table
{
    uint32_t num;
    uint32_t size;
    uint32_t addr;
    uint32_t shndx;
};

struct multiboot_info
{
  /* Multiboot info version number */
  uint32_t flags;

  /* Available memory from BIOS */
  uint32_t mem_lower;
  uint32_t mem_upper;

  /* "root" partition */
  uint32_t boot_device;

  /* Kernel command line */
  uint32_t cmdline;

  /* Boot-Module list */
  uint32_t mods_count;
  uint32_t mods_addr;

  union
  {
    struct multiboot_aout_symbol_table aout_sym;
    struct multiboot_elf_section_header_table elf_sec;
  } u;

  /* Memory Mapping buffer */
  uint32_t mmap_length;
  uint32_t mmap_addr;

  /* Drive Info buffer */
  uint32_t drives_length;
  uint32_t drives_addr;

  /* ROM configuration table */
  uint32_t config_table;

  /* Boot Loader Name */
  uint32_t boot_loader_name;

  /* APM table */
  uint32_t apm_table;

  /* Video */
  uint32_t vbe_control_info;
  uint32_t vbe_mode_info;
  uint16_t vbe_mode;
  uint16_t vbe_interface_seg;
  uint16_t vbe_interface_off;
  uint16_t vbe_interface_len;
};

struct multiboot_mmap_entry
{
  uint32_t size;
  uint32_t addr_lo;
  uint32_t addr_hi;
  uint32_t len_lo;
  uint32_t len_hi;
#define MULTIBOOT_MEMORY_AVAILABLE              1
#define MULTIBOOT_MEMORY_RESERVED               2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE       3
#define MULTIBOOT_MEMORY_NVS                    4
#define MULTIBOOT_MEMORY_BADRAM                 5
  int32_t type;
} __attribute__((packed));