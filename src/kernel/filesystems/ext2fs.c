#include <filesystems.h>
#include <string.h>
#include <util.h>

ext2_superblock_ext_t *ext2_get_superblock(uint8_t drive)
{
    printf("Attempting to read ext2 Superblock from drive %d...\n", drive);

    unsigned int *superBlockBuffer = kmalloc(SUPERBLOCK_SIZE);
    if (!superBlockBuffer)
    {
        printf("Superblock Read Error: Failed to allocate memory.\n");
        return NULL;
    }

    uint8_t error = ide_ata_rw(0, drive, 2, 2, 0x10, (unsigned int)superBlockBuffer);

    if (error != 0)
    {
        printf("Superblock Read Error: ideAtaRead failed with code %d.\n", error);
        kfree(superBlockBuffer);
        return NULL;
    }

    ext2_superblock_ext_t *superblock = (ext2_superblock_ext_t *)superBlockBuffer;
    printf("ext2 signature: 0x%x\n", superblock->signature);
    printf("%i.%i\n", superblock->majorPortionVersion, superblock->minorPortionVersion);
    printf("iNode Size: %i\n", superblock->iNodeSize);

    return superblock;
}

/**
 * Reads a specific data block from an inode, handling direct and indirect blocks.
 *
 * @param drive The drive number.
 * @param superblock The ext2 superblock.
 * @param bgdt The block group descriptor table.
 * @param inode The inode to read from.
 * @param blockNum The logical block number within the file/directory.
 * @param buffer A pointer to a buffer where the block data will be stored.
 * @return 0 on success, a non-zero error code on failure.
 */
int ext2_read_inode_block(uint8_t drive, ext2_superblock_ext_t *superblock, ext2_inode_t *inode, uint32_t blockNum, uint8_t *buffer)
{
    uint32_t blockSize = 1024 << superblock->logBlockSize;
    uint32_t pointersPerBlock = blockSize / sizeof(uint32_t);

    uint32_t blockAddress = 0;

    if (blockNum < 12)
    {
        blockAddress = inode->dbPtrs[blockNum];
    }

    else if (blockNum < (12 + pointersPerBlock))
    {
        uint32_t *singlyIndirectBlock = kmalloc(blockSize);
        if (!singlyIndirectBlock)
            return -1;

        uint32_t lba = (inode->singleIndirectBlckPtr * blockSize) / 512;
        uint32_t secsRead = blockSize / 512;
        if (ide_ata_rw(0, drive, lba, secsRead, 0x10, (unsigned int)singlyIndirectBlock) != 0)
        {
            kfree(singlyIndirectBlock);
            return -1;
        }

        blockAddress = singlyIndirectBlock[blockNum - 12];
        kfree(singlyIndirectBlock);
    }

    else if (blockNum < (12 + pointersPerBlock + (pointersPerBlock * pointersPerBlock)))
    {
        uint32_t *doublyIndirectBlock = kmalloc(blockSize);
        if (!doublyIndirectBlock)
            return -1;

        uint32_t lba = (inode->doubleIndirectBlckPtr * blockSize) / 512;
        uint32_t secsRead = blockSize / 512;
        if (ide_ata_rw(0, drive, lba, secsRead, 0x10, (unsigned int)doublyIndirectBlock) != 0)
        {
            kfree(doublyIndirectBlock);
            return -1;
        }

        uint32_t singlyBlockIndex = (blockNum - 12 - pointersPerBlock) / pointersPerBlock;
        uint32_t singlyBlockAddress = doublyIndirectBlock[singlyBlockIndex];
        kfree(doublyIndirectBlock);

        uint32_t *singlyIndirectBlock = kmalloc(blockSize);
        if (!singlyIndirectBlock)
            return -1;

        lba = (singlyBlockAddress * blockSize) / 512;
        if (ide_ata_rw(0, drive, lba, secsRead, 0x10, (unsigned int)singlyIndirectBlock) != 0)
        {
            kfree(singlyIndirectBlock);
            return -1;
        }

        uint32_t directBlockIndex = (blockNum - 12 - pointersPerBlock) % pointersPerBlock;
        blockAddress = singlyIndirectBlock[directBlockIndex];
        kfree(singlyIndirectBlock);
    }

    else
    {
        uint32_t *triplyIndirectBlock = kmalloc(blockSize);
        if (!triplyIndirectBlock)
            return -1;

        uint32_t lba = (inode->tripleIndirectBlckPtr * blockSize) / 512;
        uint32_t secsRead = blockSize / 512;
        if (ide_ata_rw(0, drive, lba, secsRead, 0x10, (unsigned int)triplyIndirectBlock) != 0)
        {
            kfree(triplyIndirectBlock);
            return -1;
        }

        uint32_t doublyBlockIndex = (blockNum - 12 - pointersPerBlock - (pointersPerBlock * pointersPerBlock)) / (pointersPerBlock * pointersPerBlock);
        uint32_t doublyBlockAddress = triplyIndirectBlock[doublyBlockIndex];
        kfree(triplyIndirectBlock);

        uint32_t *doublyIndirectBlock = kmalloc(blockSize);
        if (!doublyIndirectBlock)
            return -1;

        lba = (doublyBlockAddress * blockSize) / 512;
        if (ide_ata_rw(0, drive, lba, secsRead, 0x10, (unsigned int)doublyIndirectBlock) != 0)
        {
            kfree(doublyIndirectBlock);
            return -1;
        }

        uint32_t singlyBlockIndex = ((blockNum - 12 - pointersPerBlock - (pointersPerBlock * pointersPerBlock)) / pointersPerBlock) % pointersPerBlock;
        uint32_t singlyBlockAddress = doublyIndirectBlock[singlyBlockIndex];
        kfree(doublyIndirectBlock);

        uint32_t *singlyIndirectBlock = kmalloc(blockSize);
        if (!singlyIndirectBlock)
            return -1;

        lba = (singlyBlockAddress * blockSize) / 512;
        if (ide_ata_rw(0, drive, lba, secsRead, 0x10, (unsigned int)singlyIndirectBlock) != 0)
        {
            kfree(singlyIndirectBlock);
            return -1;
        }

        uint32_t directBlockIndex = (blockNum - 12 - pointersPerBlock - (pointersPerBlock * pointersPerBlock)) % pointersPerBlock;
        blockAddress = singlyIndirectBlock[directBlockIndex];
        kfree(singlyIndirectBlock);
    }

    if (blockAddress == 0)
    {

        memset(buffer, 0, blockSize);
        return 0;
    }

    uint32_t lba = (blockAddress * blockSize) / 512;
    uint32_t secsRead = blockSize / 512;
    return ide_ata_rw(0, drive, lba, secsRead, 0x10, (unsigned int)buffer);
}

ext2_blockgroupdescriptor_t *ext2_get_bgdt(uint8_t drive, ext2_superblock_ext_t *superblock)
{
    uint32_t blockSize = 1024 << superblock->logBlockSize;
    unsigned int *bgdtBuffer = kmalloc(blockSize);
    unsigned int bgdtBlock;

    if (!bgdtBuffer)
    {
        printf("Block Group Descriptor Table Read Error: Failed to allocate memory.\n");
        return NULL;
    }

    if (blockSize == 1024)
        bgdtBlock = 2;
    else
        bgdtBlock = 1;

    uint32_t secsRead = blockSize / 512;
    uint32_t lba = (bgdtBlock * blockSize) / 512;
    uint8_t error = ide_ata_rw(0, drive, lba, secsRead, 0x10, (unsigned int)bgdtBuffer);
    if (error != 0)
    {
        printf("Block Group Descriptor Table Read Error: ideAtaRead failed with code %d.\n", error);
        kfree(bgdtBuffer);
        return NULL;
    }

    return (ext2_blockgroupdescriptor_t *)bgdtBuffer;
}

ext2_inode_t *ext2_get_inode(uint8_t drive, ext2_superblock_ext_t *superblock, ext2_blockgroupdescriptor_t *bgdt, unsigned int inodeNum)
{
    uint32_t blockSize = 1024 << superblock->logBlockSize;
    uint32_t blockGroup = (inodeNum - 1) / superblock->iNodesPerGroup;
    ext2_blockgroupdescriptor_t target_bgd = bgdt[blockGroup];
    uint32_t iNodeTableStartBlock = target_bgd.iNodeTableStartAddr;
    uint32_t index = (inodeNum - 1) % superblock->iNodesPerGroup;
    uint32_t block_offset = (index * superblock->iNodeSize) / blockSize;
    uint32_t absolute_block = iNodeTableStartBlock + block_offset;
    uint32_t secsRead = blockSize / 512;
    uint32_t lba = (absolute_block * blockSize) / 512;
    uint8_t *nodeBuffer = kmalloc(blockSize);

    if (!nodeBuffer)
    {
        printf("inode Read Error: Failed to allocate memory.\n");
        return NULL;
    }
    uint8_t error = ide_ata_rw(0, drive, lba, secsRead, 0x10, (unsigned int)nodeBuffer);
    if (error != 0)
    {
        printf("inode Read Error: ideAtaRead failed with code %d.\n", error);
        kfree(nodeBuffer);
        return NULL;
    }

    uint32_t offset_in_block = (index * superblock->iNodeSize) % blockSize;
    ext2_inode_t *inode = (ext2_inode_t *)(nodeBuffer + offset_in_block);
    ext2_inode_t *inodeR = kmalloc(superblock->iNodeSize);

    memcpy(inodeR, inode, superblock->iNodeSize);
    kfree(inode);

    return inodeR;
}

void ext2_iterate_directory(uint8_t drive, ext2_superblock_ext_t *superblock, ext2_inode_t *inode, bool dirEntryHasType)
{
    uint32_t blockSize = 1024 << superblock->logBlockSize;

    uint16_t typePerm = inode->typePerm;
    ext2_enums_inodetype_t nodeType = (typePerm >> 12) & 0x0F;
    printf("Type Perm: 0x%x\n", nodeType);
    if (nodeType != iDIR)
    {
        printf("inode Read Error: This isnt a directory.");
        return;
    }

    for (int i = 0; i < 12; i++)
    {
        uint32_t blockAddr = inode->dbPtrs[i];

        if (blockAddr == 0)
            continue;

        printf("dbPtr %i: 0x%x\n", i, blockAddr);
        unsigned int *blockBuffer = kmalloc(blockSize);

        if (!blockBuffer)
        {
            printf("Directory Read Error: Failed to allocate memory.\n");
            return;
        }

        uint32_t secsRead = blockSize / 512;
        uint32_t lba = (blockAddr * blockSize) / 512;

        uint8_t error = ide_ata_rw(0, drive, lba, secsRead, 0x10, (unsigned int)blockBuffer);
        if (error != 0)
        {
            printf("Directory Read Error: ideAtaRead failed with code %d.\n", error);
            kfree(blockBuffer);
            return;
        }

        uint32_t offset = 0;
        uint8_t *buffer = (uint8_t *)blockBuffer;

        while (offset < blockSize)
        {
            ext2_dir_t *dirEntry = (ext2_dir_t *)(buffer + offset);
            uint8_t fileType;
            char *typeString;
            char *fileName;
            if (dirEntry->totalSize == 0)
            {
                printf("Directory Read Error: Invalid record length 0.\n");
                break;
            }

            if (dirEntry->iNode != 0)
            {
                if (dirEntryHasType)
                {
                    fileType = dirEntry->typeOrNameLengthMSB;
                }

                if (fileType == DIR)
                    typeString = "Directory";
                else if (fileType == FILE)
                    typeString = "File";
                else
                    typeString = "Unknown";

                char fileName[dirEntry->nameLengthLSB + 1];
                memcpy(fileName, dirEntry->nameCharacters, dirEntry->nameLengthLSB);
                fileName[dirEntry->nameLengthLSB] = '\0';
                printf("Name: %s iNode=%i, totalSize=%i, nameLen=%i, type=%s\n",
                       fileName,
                       dirEntry->iNode,
                       dirEntry->totalSize,
                       dirEntry->nameLengthLSB,
                       typeString);
            }

            offset += dirEntry->totalSize;
        }

        kfree(blockBuffer);
    }
}

/**
 * Reads the entire contents of a file into a newly allocated buffer.
 *
 * @param drive The drive number.
 * @param superblock The ext2 superblock.
 * @param inode The inode of the file to read.
 * @return A pointer to a buffer containing the file data, or NULL on failure.
 *         The caller is responsible for freeing this buffer with kfree().
 */
uint8_t *ext2_read_file(uint8_t drive, ext2_superblock_ext_t *superblock, ext2_inode_t *inode)
{

    uint16_t typePerm = inode->typePerm;
    if (((typePerm >> 12) & 0x0F) != iFILE)
    {
        printf("Read Error: Provided inode is not a regular file.\n");
        return NULL;
    }

    uint32_t fileSize = inode->size_lo;
    if (fileSize == 0)
    {

        uint8_t *emptyBuffer = kmalloc(1);
        if (emptyBuffer)
            emptyBuffer[0] = '\0';
        return emptyBuffer;
    }

    uint32_t blockSize = 1024 << superblock->logBlockSize;

    uint8_t *fileBuffer = kmalloc(fileSize + 1);
    if (!fileBuffer)
    {
        printf("File Read Error: Failed to allocate memory for file content.\n");
        return NULL;
    }

    uint8_t *blockBuffer = kmalloc(blockSize);
    if (!blockBuffer)
    {
        printf("File Read Error: Failed to allocate memory for a temporary block.\n");
        kfree(fileBuffer);
        return NULL;
    }

    uint32_t total_blocks = (fileSize + blockSize - 1) / blockSize;
    uint32_t bytesRemaining = fileSize;

    for (uint32_t i = 0; i < total_blocks; i++)
    {

        if (ext2_read_inode_block(drive, superblock, inode, i, blockBuffer) != 0)
        {
            printf("File Read Error: Failed to read block %u.\n", i);
            kfree(blockBuffer);
            kfree(fileBuffer);
            return NULL;
        }

        uint32_t bytesToCopy = (bytesRemaining > blockSize) ? blockSize : bytesRemaining;

        memcpy(fileBuffer + (i * blockSize), blockBuffer, bytesToCopy);

        bytesRemaining -= bytesToCopy;
    }

    fileBuffer[fileSize] = '\0';

    kfree(blockBuffer);
    return fileBuffer;
}

/*
 * Finds an entry within a directory by its name. This version correctly handles
 * directories that span multiple blocks, including indirect blocks.
 *
 * @param drive The drive number.
 * @param superblock The ext2 superblock.
 * @param dir_inode The inode of the directory to search within.
 * @param filename The name of the file to find.
 * @return The inode number of the file if found, otherwise 0.
 */
uint32_t ext2_find_entry(uint8_t drive, ext2_superblock_ext_t *superblock, ext2_inode_t *dir_inode, const char *filename)
{
    uint32_t blockSize = 1024 << superblock->logBlockSize;

    // First, verify that the provided inode is actually a directory.
    if (((dir_inode->typePerm >> 12) & 0x0F) != iDIR)
    {
        printf("Find File Error: The provided inode is not a directory.\n");
        return 0; // 0 indicates file not found
    }

    // Calculate how many data blocks this directory uses based on its size
    uint32_t total_blocks = (dir_inode->size_lo + blockSize - 1) / blockSize;

    uint8_t *blockBuffer = kmalloc(blockSize);
    if (!blockBuffer)
    {
        printf("Find File Error: Failed to allocate memory for a block.\n");
        return 0;
    }

    // Iterate through ALL logical blocks of the directory
    for (uint32_t i = 0; i < total_blocks; i++)
    {
        // Use the existing function to read a block from the inode.
        // This function correctly handles direct and indirect blocks.
        if (ext2_read_inode_block(drive, superblock, dir_inode, i, blockBuffer) != 0)
        {
            printf("Find File Error: Failed to read block %u.\n", i);
            kfree(blockBuffer);
            return 0;
        }

        uint32_t offset = 0;
        while (offset < blockSize)
        {
            ext2_dir_t *dirEntry = (ext2_dir_t *)(blockBuffer + offset);

            if (dirEntry->totalSize == 0)
            {
                break; // End of directory entries in this block
            }

            // Check if this entry is in use and if the filename matches.
            if (dirEntry->iNode != 0 && strlen(filename) == dirEntry->nameLengthLSB)
            {
                if (memcmp(filename, dirEntry->nameCharacters, dirEntry->nameLengthLSB) == 0)
                {
                    uint32_t inodeNum = dirEntry->iNode;
                    kfree(blockBuffer);
                    return inodeNum; // File found, return its inode number.
                }
            }
            offset += dirEntry->totalSize;
        }
    }

    kfree(blockBuffer);
    return 0; // File not found after checking all blocks.
}

/**
 * Finds a file or directory by its full path starting from root
 */
ext2_inode_t *ext2_find_by_path(uint8_t drive, ext2_superblock_ext_t *superblock, ext2_blockgroupdescriptor_t *bgdt, const char *path)
{
    char *path_copy = strdup(path);
    if (!path_copy)
    {
        printf("Error Copying name\n");
        return NULL;
    }

    if (path_copy[0] != '/')
    {
        printf("Find By Path Error: Path must be absolute (start with '/').\n");
        kfree(path_copy);
        return NULL;
    }

    uint32_t cur_inode_num = 2;
    ext2_inode_t *cur_inode = ext2_get_inode(drive, superblock, bgdt, cur_inode_num);
    if (!cur_inode)
    {
        printf("Find By Path Error: Could not parse root directory inode.\n");
        kfree(path_copy);
        return NULL;
    }

    char *saveptr;
    char *token = strtok_r(path_copy, "/", &saveptr);

    while (token != NULL)
    {
        uint32_t next_inode_num = ext2_find_entry(drive, superblock, cur_inode, token);

        kfree(cur_inode);

        if (next_inode_num == 0)
        {
            // Entry not found
            printf("Find By Path Error: Could not find '%s'.\n", token);
            kfree(path_copy);
            return NULL;
        }

        cur_inode_num = next_inode_num;
        cur_inode = ext2_get_inode(drive, superblock, bgdt, cur_inode_num);
        if (!cur_inode)
        {
            printf("Find By Path Error: Failed to parse inode %u for token '%s'.\n", cur_inode_num, token);
            kfree(path_copy);
            return NULL;
        }

        // Get the next part of the path
        token = strtok_r(NULL, "/", &saveptr);

        // If there are more parts to the path, the current entry MUST be a directory
        if (token != NULL)
        {
            if (((cur_inode->typePerm >> 12) & 0x0F) != iDIR)
            {
                printf("Find By Path Error: Path component is not a directory.\n");
                kfree(cur_inode);
                kfree(path_copy);
                return NULL;
            }
        }
    }

    // If we've processed the whole path, current_inode_num is our result.
    kfree(cur_inode); // Clean up the last inode we parsed
    kfree(path_copy); // Clean up the path string copy
    return ext2_get_inode(drive, superblock, bgdt, cur_inode_num);
}

int ext2_allocate_block(uint8_t drive, ext2_superblock_ext_t *superblock, ext2_blockgroupdescriptor_t *bgdt)
{
    uint32_t blockSize = 1024 << superblock->logBlockSize;
    uint8_t *bitmap = kmalloc(blockSize);
    if (!bitmap)
    {
        printf("Block bitmap Read Error: Failed to allocate memory.\n");
        return -1;
    }

    uint32_t secsRead = blockSize / 512;
    uint32_t lba = (bgdt->blockUsageBitmapAddr * blockSize) / 512;
    uint8_t error = ide_ata_rw(0, drive, lba, secsRead, 0x10, (unsigned int)bitmap);

    if (error != 0)
    {
        printf("Block bitmap Read Error: ideAtaRead failed with code %d.\n", error);
        kfree(bitmap);
        return -1;
    }
    // hexdump(bitmap, 128);
    int alloc = 0;

    for (int i = 0; i < blockSize; i++)
    {
        if (bitmap[i] == 0xFF)
            continue;
        printf("Found first non-fully allocated block at index %i\n", i);
        alloc = i;
        break;
    }

    int allocated_index;
    for (int i = 0; i < 8; i++)
    {
        uint8_t mask = 1 << i;
        if ((bitmap[alloc] & mask) == 0)
        {

            bitmap[alloc] |= mask;

            allocated_index = (alloc * 8) + i;
            break;
        }
    }

    bgdt->free_blocks_count--;
    superblock->free_blocks_count--;

    return allocated_index;
}

int ext2_create_entry(uint8_t drive, ext2_superblock_ext_t *superblock, ext2_blockgroupdescriptor_t *bgdt, uint32_t new_inode_num, ext2_inode_t *parent, uint32_t parent_num, char *name, ext2_dirtype_t type)
{
    uint16_t required_size = 8 + strlen(name);

    // Round up to the nearest multiple of 4
    if (required_size % 4 != 0)
        required_size = (required_size + 3) & ~3; // A common alignment trick

    uint32_t blockSize = 1024 << superblock->logBlockSize;
    uint32_t totalBlocks = parent->size_lo / blockSize;
    uint8_t *blockBuffer = kmalloc(blockSize);

    for (uint32_t i = 0; i < totalBlocks; i++)
    {
        // Read the directory's data block into memory
        ext2_read_inode_block(drive, superblock, parent, i, blockBuffer);

        uint32_t offset = 0;
        while (offset < blockSize)
        {
            ext2_dir_t *current_entry = (ext2_dir_t *)(blockBuffer + offset);

            // If iNode is 0, this is an unused entry, but we can't rely on that.
            // The key is the totalSize.
            if (current_entry->totalSize == 0)
            {
                // Corrupt entry, stop.
                break;
            }

            // 1. Calculate the ideal size of the CURRENT entry (aligned)
            uint16_t ideal_size_of_current = 8 + current_entry->nameLengthLSB;
            if (ideal_size_of_current % 4 != 0)
            {
                ideal_size_of_current = (ideal_size_of_current + 3) & ~3;
            }

            // 2. Calculate the "slack" or "padding" space in the current entry
            uint16_t slack_space = current_entry->totalSize - ideal_size_of_current;

            // 3. CHECK: Is there enough slack space for our new entry?
            if (slack_space >= required_size)
            {
                // --- SPACE FOUND! ---
                // a. Shrink the current entry to its ideal size.
                current_entry->totalSize = ideal_size_of_current;

                // b. Create the new entry right after it.
                ext2_dir_t *new_entry = (ext2_dir_t *)(blockBuffer + offset + ideal_size_of_current);
                new_entry->iNode = new_inode_num;
                new_entry->nameLengthLSB = strlen(name);
                new_entry->typeOrNameLengthMSB = type; // Assuming you have dir entry types enabled
                memcpy(new_entry->nameCharacters, name, strlen(name));

                // c. The new entry's totalSize takes up the rest of the slack space.
                new_entry->totalSize = slack_space;

                // d. Write the modified block back to the disk.
                // You'll need an ext2WriteInodeBlock function (the reverse of read).
                // ext2WriteInodeBlock(drive, superblock, parent_inode, i, blockBuffer);

                // e. Clean up and return SUCCESS.
                kfree(blockBuffer);
                return 0; // Success
            }

            // If not enough space, move to the next entry
            offset += current_entry->totalSize;
        }

        // no entry could be split uh oh time to allocate
        // uint32_t newBlockAddr = ext2_allocate_block(drive, superblock, bgdt);
    }
}
void ext2_read_drive(uint8_t drive)
{
    ext2_superblock_ext_t *superblock = ext2_get_superblock(drive);
    ext2_blockgroupdescriptor_t *bgdt = ext2_get_bgdt(drive, superblock);
    bool dirEntryHasType;

    if (superblock->requiredFlags & REQ_DIR_TYPE_FIELD)
        dirEntryHasType = true;
    if (superblock->requiredFlags & REQ_JOURNAL_DEVICE)
        printf("File system uses a journal device\n");
    if (superblock->requiredFlags & REQ_COMPRESSION)
        printf("Compression is used\n");
    if (superblock->requiredFlags & REQ_JOURNAL_DEVICE)
        printf("File system uses a journal device\n");
    if (dirEntryHasType)
        printf("Directory entries contain a type field\n");

    if (CEIL_DIV(superblock->total_blocks, superblock->blocksPerGroup) != CEIL_DIV(superblock->total_inodes, superblock->iNodesPerGroup))
        printf("EXT2: Error: Total blocks / Blocks per group != Total iNodes / iNodes\n");

    ext2_inode_t *file = ext2_find_by_path(drive, superblock, bgdt, "/test.txt");
    uint8_t *buffer = ext2_read_file(drive, superblock, file);
    char *fileText = (char *)buffer;
    printf("File Contents:\n%s", fileText);
    printf("allocating first unallocated block\n");
    int groupIndex = (2 - 1) / superblock->iNodesPerGroup;
    printf("groupindex of root dir %i\n", groupIndex);
    int alloc_index = ext2_allocate_block(drive, superblock, bgdt);
    kfree(bgdt);
    kfree(superblock);
}