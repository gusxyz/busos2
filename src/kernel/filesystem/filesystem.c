#include "filesystem.h"
#include "../drivers/ide.h"
#include "../liballoc/liballoc.h"
#include "../stdlib/stdio.h"

int isoFilenameCompare(const char *isoName, int isoLen, const char *cName)
{
    int c_len = strlen(cName);

    // Check for a version number like ";1" in the ISO name
    if (isoLen > 2 && isoName[isoLen - 2] == ';')
    {
        isoLen -= 2;
    }

    if (isoLen != c_len)
    {
        return 1; // Lengths don't match
    }

    // Perform a case-insensitive comparison
    for (int i = 0; i < c_len; i++)
    {
        // A simple toupper() would be great here, but this works too.
        char c1 = isoName[i];
        char c2 = cName[i];
        if (c1 >= 'a' && c1 <= 'z')
            c1 -= 32;
        if (c2 >= 'a' && c2 <= 'z')
            c2 -= 32;
        if (c1 != c2)
            return 1;
    }

    return 0; // Match
}

/**
 * @brief Searches a directory's contents for an entry with a given name.
 * @param drive The drive to read from.
 * @param dirLBA The LBA where the directory's data starts.
 * @param name The name of the file or subdirectory to find.
 * @return A pointer to a kmalloc'd directory entry struct on success, NULL on failure.
 */
iso9660_dir_t *resolveEntry(uint8_t drive, uint32_t dirLBA, const char *name)
{
    uint8_t *dirContentBuf = NULL;
    uint32_t dirSize = 0;

    // This block is similar to your // printfilesInDirectory function.
    // It reads the entire directory contents into a buffer.
    {
        uint8_t *sectorBuffer = (uint8_t *)kmalloc(ISO_BLOCKSIZE);
        if (!sectorBuffer)
            return NULL;

        if (ideAtapiRead(drive, dirLBA, 1, 0x10, (unsigned int)sectorBuffer))
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
            if (ideAtapiRead(drive, dirLBA, numSectors, 0x10, (unsigned int)dirContentBuf))
            {
                kfree(dirContentBuf);
                return NULL;
            }
        }
    }

    // Now, loop through the directory entries to find a match.
    uint32_t offset = 0;
    while (offset < dirSize)
    {
        iso9660_dir_t *entry = (iso9660_dir_t *)(dirContentBuf + offset);
        if (entry->length == 0)
            break;

        // Use our special comparison function
        if (iso_filename_compare(entry->filename.str, entry->filename.len, name) == 0)
        {
            // Found a match!
            // Allocate a new struct to hold the result.
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
    return NULL; // Entry not found
}
iso9660_pvd_t *getPVDStruct(uint8_t driveIndex)
{
    uint32_t pvdLBA = ISO_PVD_SECTOR;
    uint32_t sectorSize = ISO_BLOCKSIZE;

    // printf("Attempting to read PVD from drive %d...\n", driveIndex);

    uint8_t *buffer = (uint8_t *)kmalloc(sectorSize);
    if (!buffer)
    {
        // printf("PVD Read Error: Failed to allocate memory.\n");
        return NULL;
    }

    uint8_t error = ideAtapiRead(driveIndex, pvdLBA, 1, 0x10, (unsigned int)buffer);

    if (error != 0)
    {
        // printf("PVD Read Error: ideAtapiRead failed with code %d.\n", error);
        kfree(buffer);
        return NULL;
    }

    iso9660_pvd_t *pvd = (iso9660_pvd_t *)buffer;

    if (pvd->type != ISO_VD_PRIMARY && strcmp(pvd->id, "CD001") != 0) // need to implement strcmp(pvd->id = cd0001) 5 != 0) add string length prob to makee strcmp better
    {
        // printf("PVD Validation Error: Invalid PVD signature.\n");
        kfree(buffer);
        return NULL;
    }

    // printf("Successfully read and validated PVD:\n");

    return pvd;
}

uint8_t *loadPathTable(uint8_t drive, iso9660_pvd_t *pvd)
{
    uint32_t pathTableLBA = pvd->type_l_path_table;
    uint32_t pathTableSize = pvd->pathTableSize;

    // printf("Path Table is at LBA %u, size is %u bytes.\n", pathTableLBA, pathTableSize);

    if (pathTableSize == 0)
    {
        // printf("Error: Path table size is zero.\n");
        return NULL;
    }

    uint32_t numSectors = (pathTableSize + ISO_BLOCKSIZE - 1) / ISO_BLOCKSIZE;

    uint8_t *tableBuffer = (uint8_t *)kmalloc(numSectors * ISO_BLOCKSIZE);
    if (!tableBuffer)
    {
        // printf("Error: Failed to allocate memory for path table.\n");
        return NULL;
    }

    uint8_t error = ideAtapiRead(drive, pathTableLBA, numSectors, 0x10, (unsigned int)tableBuffer);
    if (error)
    {
        // printf("Error: Failed to read path table from disc. Code: %d\n", error);
        kfree(tableBuffer);
        return NULL;
    }

    return tableBuffer;
}

void printDirectoryRecursive(uint8_t drive, uint8_t *pathTableBuf, uint32_t table_size, uint16_t parent_index, int depth)
{
    uint32_t table_offset = 0;
    uint16_t current_entry_index = 0;

    // We must scan the entire table from the beginning for each level to find all children of the current parent.
    while (table_offset < table_size)
    {
        current_entry_index++;
        iso9660_pathtable_t *pt_entry = (iso9660_pathtable_t *)(pathTableBuf + table_offset);

        // Check if this entry is a direct child of the directory we're interested in.
        // We also check that it's not the parent pointing to itself (which the root does).
        if (pt_entry->parent_di_no == parent_index && current_entry_index != parent_index)
        {

            // --- This entry is a child, so print it ---

            // 1. Print indentation to create the tree structure
            for (int i = 0; i < depth; i++)
            {
                // printf("  "); // Print two spaces per depth level
            }
            // printf("|--");

            // 2. Print the directory name (which is not null-terminated)
            for (int i = 0; i < pt_entry->len_di; i++)
            {
                // printf("%c", pt_entry->identifier[i]);
            }
            // printf("\n");
            // printfilesInDirectory(drive, pt_entry->extent, depth);

            // 3. This child is a directory, so recurse to print its contents.
            // The new parent is the index of the entry we just found.
            // The depth increases by 1.
            printDirectoryRecursive(drive, pathTableBuf, table_size, current_entry_index, depth + 1);
        }

        table_offset += sizeof(iso9660_pathtable_t) - 1 + pt_entry->len_di;
        if (pt_entry->len_di % 2 == 1)
        {
            table_offset++;
        }
    }
}

void // printfileSystemTree(uint8_t drive)
{
    // printf("Attempting to print filesystem tree for drive %d:\n", drive);

    iso9660_pvd_t *pvd = getPVDStruct(drive);
    if (!pvd)
    {
        // printf("Failed to read PVD. Aborting.\n");
        return;
    }

    uint8_t *path_table = loadPathTable(drive, pvd);
    if (!path_table)
    {
        // printf("Failed to load path table. Aborting.\n");
        kfree(pvd);
        return;
    }

    uint32_t pt_size = pvd->pathTableSize;

    kfree(pvd);

    // printf("/\n");
    printDirectoryRecursive(drive, path_table, pt_size, 1, 0);

    kfree(path_table);

    // printf("\nFilesystem tree printed.\n");
}

void // printfilesInDirectory(uint8_t drive, uint32_t dirLBA, int depth)
{
    // Read the first sector to determine the full size of the directory listing.
    uint8_t *sectorBuffer = (uint8_t *)kmalloc(ISO_BLOCKSIZE);
    if (!sectorBuffer)
        return;

    if (ideAtapiRead(drive, dirLBA, 1, 0x10, (unsigned int)sectorBuffer))
    {
        // printf("Read error in // printfilesInDirectory\n");
        kfree(sectorBuffer);
        return;
    }

    // The first entry in a directory is the "." entry, which holds the total size.
    iso9660_dir_t *self_entry = (iso9660_dir_t *)sectorBuffer;
    uint32_t dirSize = self_entry->size;

    // Now that we have the real size, we might need a bigger buffer.
    uint8_t *dirContentBuf = NULL;
    uint32_t num_sectors = (dirSize + ISO_BLOCKSIZE - 1) / ISO_BLOCKSIZE;

    if (num_sectors <= 1)
    {
        // The whole directory fits in the sector we already read. No need to re-read.
        dirContentBuf = sectorBuffer;
    }
    else
    {
        // The directory spans multiple sectors. Free the small buffer,
        // allocate a larger one, and read all sectors.
        kfree(sectorBuffer);
        dirContentBuf = (uint8_t *)kmalloc(num_sectors * ISO_BLOCKSIZE);
        if (!dirContentBuf)
            return;

        if (ideAtapiRead(drive, dirLBA, num_sectors, 0x10, (unsigned int)dirContentBuf))
        {
            // printf("Read error for multi-sector directory\n");
            kfree(dirContentBuf);
            return;
        }
    }

    // Now we have the full directory content in dir_content_buf. Let's parse it.
    uint32_t offset = 0;
    while (offset < dirSize)
    {
        iso9660_dir_t *entry = (iso9660_dir_t *)(dirContentBuf + offset);

        if (entry->length == 0)
        {
            // End of records in this sector.
            break;
        }

        // We only want to print files. A record is a file if the DIRECTORY flag is NOT set.
        // We also skip the "." and ".." entries.
        if (!(entry->file_flags & ISO_DIRECTORY))
        {
            // Print indentation for a file (one level deeper than its parent dir)
            for (int i = 0; i < depth + 1; i++)
            {
                // printf(" ");
            }
            // printf("- "); // Use a different prefix for files

            // Print the filename
            for (int i = 0; i < entry->filename.len; i++)
            {
                // printf("%c", entry->filename.str[i]);
            }
            // printf("\n");
        }

        // Move to the next entry
        offset += entry->length;
    }

    // Clean up the buffer we used.
    kfree(dirContentBuf);
}
