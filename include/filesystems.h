#ifndef FILESYSTEMS_H
#define FILESYSTEMS_H
#include <iso9660.h>
#include <ext2.h>
#include <stdbool.h>
#include <drivers/ide.h>
#include <liballoc.h>
#include <stdio.h>
#include <util.h>

// vfs

// ext2
void extReadDrive(uint8_t drive);

// iso 9660
iso9660_pvd_t *isoGetPVDStruct(uint8_t driveIndex);
uint8_t *isoLoadPathTable(uint8_t drive, iso9660_pvd_t *pvd);
int isoFilenameCompare(const char *isoName, int isoLen, const char *cName);
iso9660_dir_t *isoResolveEntry(uint8_t drive, uint32_t dirLBA, const char *name);
void isoPrintDirectoryRecursive(uint8_t drive, uint8_t *path_table_buf, uint32_t table_size, uint16_t parent_index, int depth);
void isoPrintfileSystemTree(uint8_t drive);
void isoPrintFilesInDirectory(uint8_t drive, uint32_t dir_lba, int depth);

typedef enum filesystem_s
{
    EXT2,
    ISO9660,
} filesystem_t;
#endif