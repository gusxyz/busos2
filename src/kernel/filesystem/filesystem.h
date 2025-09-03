#pragma once
#include "iso9660.h"

iso9660_pvd_t *getPVDStruct(uint8_t driveIndex);
uint8_t *loadPathTable(uint8_t drive, iso9660_pvd_t *pvd);
int isoFilenameCompare(const char *isoName, int isoLen, const char *cName);
iso9660_dir_t *resolveEntry(uint8_t drive, uint32_t dirLBA, const char *name);
void printDirectoryRecursive(uint8_t drive, uint8_t *path_table_buf, uint32_t table_size, uint16_t parent_index, int depth);
void printfileSystemTree(uint8_t drive);
void printfilesInDirectory(uint8_t drive, uint32_t dir_lba, int depth);