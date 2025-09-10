#include <filesystems.h>

int isoFilenameCompare(const char *isoName, int isoLen, const char *cName)
{
    int c_len = strlen(cName);

    if (isoLen > 2 && isoName[isoLen - 2] == ';')
    {
        isoLen -= 2;
    }

    if (isoLen != c_len)
    {
        return 1;
    }

    for (int i = 0; i < c_len; i++)
    {

        char c1 = isoName[i];
        char c2 = cName[i];
        if (c1 >= 'a' && c1 <= 'z')
            c1 -= 32;
        if (c2 >= 'a' && c2 <= 'z')
            c2 -= 32;
        if (c1 != c2)
            return 1;
    }

    return 0;
}

/**
 * @brief Searches a directory's contents for an entry with a given name.
 * @param drive The drive to read from.
 * @param dirLBA The LBA where the directory's data starts.
 * @param name The name of the file or subdirectory to find.
 * @return A pointer to a kmalloc'd directory entry struct on success, NULL on failure.
 */
iso9660_dir_t *isoResolveEntry(uint8_t drive, uint32_t dirLBA, const char *name)
{
    uint8_t *dirContentBuf = NULL;
    uint32_t dirSize = 0;

    {
        uint8_t *sectorBuffer = (uint8_t *)kmalloc(ISO_BLOCKSIZE);
        if (!sectorBuffer)
            return NULL;

        if (ide_atapi_read(drive, dirLBA, 1, 0x10, (unsigned int)sectorBuffer))
        {
            kfree(sectorBuffer);
            return NULL;
        }

        iso9660_dir_t *self_entry = (iso9660_dir_t *)sectorBuffer;
        dirSize = self_entry->size;
        uint32_t numSectors = (dirSize + ISO_BLOCKSIZE - 1) / ISO_BLOCKSIZE;

        if (numSectors <= 1)
        {
            dirContentBuf = sectorBuffer;
        }
        else
        {
            kfree(sectorBuffer);
            dirContentBuf = (uint8_t *)kmalloc(numSectors * ISO_BLOCKSIZE);
            if (!dirContentBuf)
                return NULL;
            if (ide_atapi_read(drive, dirLBA, numSectors, 0x10, (unsigned int)dirContentBuf))
            {
                kfree(dirContentBuf);
                return NULL;
            }
        }
    }

    uint32_t offset = 0;
    while (offset < dirSize)
    {
        iso9660_dir_t *entry = (iso9660_dir_t *)(dirContentBuf + offset);
        if (entry->length == 0)
            break;

        if (isoFilenameCompare(entry->filename.str, entry->filename.len, name) == 0)
        {

            iso9660_dir_t *result = (iso9660_dir_t *)kmalloc(sizeof(iso9660_dir_t));
            if (result)
            {
                memcpy(result, entry, sizeof(iso9660_dir_t));
            }
            kfree(dirContentBuf);
            return result;
        }
        offset += entry->length;
    }

    kfree(dirContentBuf);
    return NULL;
}

iso9660_pvd_t *isoGetPVDStruct(uint8_t driveIndex)
{
    uint32_t pvdLBA = ISO_PVD_SECTOR;
    uint32_t sectorSize = ISO_BLOCKSIZE;

    printf("Attempting to read PVD from drive %d...\n", driveIndex);

    uint8_t *buffer = (uint8_t *)kmalloc(sectorSize);
    if (!buffer)
    {
        printf("PVD Read Error: Failed to allocate memory.\n");
        return NULL;
    }

    uint8_t error = ide_atapi_read(driveIndex, pvdLBA, 1, 0x10, (unsigned int)buffer);

    if (error != 0)
    {
        printf("PVD Read Error: ideAtapiRead failed with code %d.\n", error);
        kfree(buffer);
        return NULL;
    }

    iso9660_pvd_t *pvd = (iso9660_pvd_t *)buffer;

    if (pvd->type != ISO_VD_PRIMARY && strcmp(pvd->id, "CD001") != 0)
    {
        printf("PVD Validation Error: Invalid PVD signature.\n");
        kfree(buffer);
        return NULL;
    }

    printf("Successfully read and validated PVD:\n");

    return pvd;
}

uint8_t *isoLoadPathTable(uint8_t drive, iso9660_pvd_t *pvd)
{
    uint32_t pathTableLBA = pvd->type_l_path_table;
    uint32_t pathTableSize = pvd->pathTableSize;

    printf("Path Table is at LBA %u, size is %u bytes.\n", pathTableLBA, pathTableSize);

    if (pathTableSize == 0)
    {
        printf("Error: Path table size is zero.\n");
        return NULL;
    }

    uint32_t numSectors = (pathTableSize + ISO_BLOCKSIZE - 1) / ISO_BLOCKSIZE;

    uint8_t *tableBuffer = (uint8_t *)kmalloc(numSectors * ISO_BLOCKSIZE);
    if (!tableBuffer)
    {
        printf("Error: Failed to allocate memory for path table.\n");
        return NULL;
    }

    uint8_t error = ide_atapi_read(drive, pathTableLBA, numSectors, 0x10, (unsigned int)tableBuffer);
    if (error)
    {
        printf("Error: Failed to read path table from disc. Code: %d\n", error);
        kfree(tableBuffer);
        return NULL;
    }

    return tableBuffer;
}

void isoPrintDirectoryRecursive(uint8_t drive, uint8_t *pathTableBuf, uint32_t table_size, uint16_t parent_index, int depth)
{
    uint32_t table_offset = 0;
    uint16_t current_entry_index = 0;

    while (table_offset < table_size)
    {
        current_entry_index++;
        iso9660_pathtable_t *pt_entry = (iso9660_pathtable_t *)(pathTableBuf + table_offset);

        if (pt_entry->parent_di_no == parent_index && current_entry_index != parent_index)
        {

            for (int i = 0; i < depth; i++)
            {
                printf("  ");
            }
            printf("|--");

            for (int i = 0; i < pt_entry->len_di; i++)
            {
                printf("%c", pt_entry->identifier[i]);
            }
            printf("\n");
            isoPrintFilesInDirectory(drive, pt_entry->extent, depth);

            isoPrintDirectoryRecursive(drive, pathTableBuf, table_size, current_entry_index, depth + 1);
        }

        table_offset += sizeof(iso9660_pathtable_t) - 1 + pt_entry->len_di;
        if (pt_entry->len_di % 2 == 1)
        {
            table_offset++;
        }
    }
}

void isoPrintfileSystemTree(uint8_t drive)
{
    printf("Attempting to print filesystem tree for drive %d:\n", drive);

    iso9660_pvd_t *pvd = isoGetPVDStruct(drive);
    if (!pvd)
    {
        printf("Failed to read PVD. Aborting.\n");
        return;
    }

    uint8_t *path_table = isoLoadPathTable(drive, pvd);
    if (!path_table)
    {
        printf("Failed to load path table. Aborting.\n");
        kfree(pvd);
        return;
    }

    uint32_t pt_size = pvd->pathTableSize;

    kfree(pvd);

    printf("/\n");
    isoPrintDirectoryRecursive(drive, path_table, pt_size, 1, 0);

    kfree(path_table);

    printf("\nFilesystem tree printed.\n");
}

void isoPrintFilesInDirectory(uint8_t drive, uint32_t dirLBA, int depth)
{

    uint8_t *sectorBuffer = (uint8_t *)kmalloc(ISO_BLOCKSIZE);
    if (!sectorBuffer)
        return;

    if (ide_atapi_read(drive, dirLBA, 1, 0x10, (unsigned int)sectorBuffer))
    {
        printf("Read error in printfilesInDirectory\n");
        kfree(sectorBuffer);
        return;
    }

    iso9660_dir_t *self_entry = (iso9660_dir_t *)sectorBuffer;
    uint32_t dirSize = self_entry->size;

    uint8_t *dirContentBuf = NULL;
    uint32_t num_sectors = (dirSize + ISO_BLOCKSIZE - 1) / ISO_BLOCKSIZE;

    if (num_sectors <= 1)
    {

        dirContentBuf = sectorBuffer;
    }
    else
    {

        kfree(sectorBuffer);
        dirContentBuf = (uint8_t *)kmalloc(num_sectors * ISO_BLOCKSIZE);
        if (!dirContentBuf)
            return;

        if (ide_atapi_read(drive, dirLBA, num_sectors, 0x10, (unsigned int)dirContentBuf))
        {
            printf("Read error for multi-sector directory\n");
            kfree(dirContentBuf);
            return;
        }
    }

    uint32_t offset = 0;
    while (offset < dirSize)
    {
        iso9660_dir_t *entry = (iso9660_dir_t *)(dirContentBuf + offset);

        if (entry->length == 0)
        {

            break;
        }

        if (!(entry->file_flags & ISO_DIRECTORY))
        {

            for (int i = 0; i < depth + 1; i++)
            {
                printf(" ");
            }
            printf("-");

            for (int i = 0; i < entry->filename.len; i++)
            {
                printf("%c", entry->filename.str[i]);
            }
            printf("\n");
        }

        offset += entry->length;
    }

    kfree(dirContentBuf);
}