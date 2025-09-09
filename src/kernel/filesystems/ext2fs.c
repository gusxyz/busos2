#include <filesystems.h>

// ext2
ext2_superblock_ext_t *extGetSuperblock(uint8_t drive)
{
    printf("Attempting to read ext2 Superblock from drive %d...\n", drive);

    unsigned int *superBlockBuffer = kmalloc(SUPERBLOCK_SIZE); // size and loc r the same.
    if (!superBlockBuffer)
    {
        printf("Superblock Read Error: Failed to allocate memory.\n");
        return NULL;
    }

    uint8_t error = ideAtaReadWrite(0, drive, 2, 2, 0x10, (unsigned int)superBlockBuffer);

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
int ext2ReadInodeBlock(uint8_t drive, ext2_superblock_ext_t *superblock, ext2_inode_t *inode, uint32_t blockNum, uint8_t *buffer)
{
    uint32_t blockSize = 1024 << superblock->logBlockSize;
    uint32_t pointersPerBlock = blockSize / sizeof(uint32_t);

    uint32_t blockAddress = 0;

    // Direct block pointers
    if (blockNum < 12)
    {
        blockAddress = inode->dbPtrs[blockNum];
    }
    // Singly indirect block pointer
    else if (blockNum < (12 + pointersPerBlock))
    {
        uint32_t *singlyIndirectBlock = kmalloc(blockSize);
        if (!singlyIndirectBlock)
            return -1;

        uint32_t lba = (inode->singleIndirectBlckPtr * blockSize) / 512;
        uint32_t secsRead = blockSize / 512;
        if (ideAtaReadWrite(0, drive, lba, secsRead, 0x10, (unsigned int)singlyIndirectBlock) != 0)
        {
            kfree(singlyIndirectBlock);
            return -1;
        }

        blockAddress = singlyIndirectBlock[blockNum - 12];
        kfree(singlyIndirectBlock);
    }
    // Doubly indirect block pointer
    else if (blockNum < (12 + pointersPerBlock + (pointersPerBlock * pointersPerBlock)))
    {
        uint32_t *doublyIndirectBlock = kmalloc(blockSize);
        if (!doublyIndirectBlock)
            return -1;

        uint32_t lba = (inode->doubleIndirectBlckPtr * blockSize) / 512;
        uint32_t secsRead = blockSize / 512;
        if (ideAtaReadWrite(0, drive, lba, secsRead, 0x10, (unsigned int)doublyIndirectBlock) != 0)
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
        if (ideAtaReadWrite(0, drive, lba, secsRead, 0x10, (unsigned int)singlyIndirectBlock) != 0)
        {
            kfree(singlyIndirectBlock);
            return -1;
        }

        uint32_t directBlockIndex = (blockNum - 12 - pointersPerBlock) % pointersPerBlock;
        blockAddress = singlyIndirectBlock[directBlockIndex];
        kfree(singlyIndirectBlock);
    }
    // Triply indirect block pointer
    else
    {
        uint32_t *triplyIndirectBlock = kmalloc(blockSize);
        if (!triplyIndirectBlock)
            return -1;

        uint32_t lba = (inode->tripleIndirectBlckPtr * blockSize) / 512;
        uint32_t secsRead = blockSize / 512;
        if (ideAtaReadWrite(0, drive, lba, secsRead, 0x10, (unsigned int)triplyIndirectBlock) != 0)
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
        if (ideAtaReadWrite(0, drive, lba, secsRead, 0x10, (unsigned int)doublyIndirectBlock) != 0)
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
        if (ideAtaReadWrite(0, drive, lba, secsRead, 0x10, (unsigned int)singlyIndirectBlock) != 0)
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
        // This block is not allocated (a sparse file, or end of file)
        memset(buffer, 0, blockSize);
        return 0;
    }

    uint32_t lba = (blockAddress * blockSize) / 512;
    uint32_t secsRead = blockSize / 512;
    return ideAtaReadWrite(0, drive, lba, secsRead, 0x10, (unsigned int)buffer);
}
ext2_blockgroupdescriptor_t *extGetBGDT(uint8_t drive, ext2_superblock_ext_t *superblock)
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
    uint8_t error = ideAtaReadWrite(0, drive, lba, secsRead, 0x10, (unsigned int)bgdtBuffer);
    if (error != 0)
    {
        printf("Block Group Descriptor Table Read Error: ideAtaRead failed with code %d.\n", error);
        kfree(bgdtBuffer);
        return NULL;
    }

    return (ext2_blockgroupdescriptor_t *)bgdtBuffer;
}

ext2_inode_t *extParseiNode(uint8_t drive, ext2_superblock_ext_t *superblock, ext2_blockgroupdescriptor_t *bgdt, unsigned int inodeNum)
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
    uint8_t error = ideAtaReadWrite(0, drive, lba, secsRead, 0x10, (unsigned int)nodeBuffer);
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

void extIterateDirectoryINode(uint8_t drive, ext2_superblock_ext_t *superblock, ext2_inode_t *inode, bool dirEntryHasType)
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

        uint8_t error = ideAtaReadWrite(0, drive, lba, secsRead, 0x10, (unsigned int)blockBuffer);
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
                fileName[dirEntry->nameLengthLSB] = '\0'; // Add the null terminator
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
uint8_t *ext2ReadFile(uint8_t drive, ext2_superblock_ext_t *superblock, ext2_inode_t *inode)
{
    // First, verify that the inode is a regular file.
    uint16_t typePerm = inode->typePerm;
    if (((typePerm >> 12) & 0x0F) != iFILE)
    {
        printf("Read Error: Provided inode is not a regular file.\n");
        return NULL;
    }

    uint32_t fileSize = inode->size_lo;
    if (fileSize == 0)
    {
        // Handle zero-byte files by returning an empty, null-terminated string.
        uint8_t *emptyBuffer = kmalloc(1);
        if (emptyBuffer)
            emptyBuffer[0] = '\0';
        return emptyBuffer;
    }

    uint32_t blockSize = 1024 << superblock->logBlockSize;

    // Allocate the main buffer to hold the entire file content.
    // We add +1 for a null terminator, which is convenient for text files.
    uint8_t *fileBuffer = kmalloc(fileSize + 1);
    if (!fileBuffer)
    {
        printf("File Read Error: Failed to allocate memory for file content.\n");
        return NULL;
    }

    // Allocate a temporary buffer to hold one block at a time.
    uint8_t *blockBuffer = kmalloc(blockSize);
    if (!blockBuffer)
    {
        printf("File Read Error: Failed to allocate memory for a temporary block.\n");
        kfree(fileBuffer);
        return NULL;
    }

    uint32_t totalBlocks = (fileSize + blockSize - 1) / blockSize;
    uint32_t bytesRemaining = fileSize;

    // Loop through each logical block of the file.
    for (uint32_t i = 0; i < totalBlocks; i++)
    {
        // Use our helper function to read the current logical block into blockBuffer.
        if (ext2ReadInodeBlock(drive, superblock, inode, i, blockBuffer) != 0)
        {
            printf("File Read Error: Failed to read block %u.\n", i);
            kfree(blockBuffer);
            kfree(fileBuffer);
            return NULL;
        }

        // Determine how many bytes to copy from this block.
        // It's either a full block or the remaining few bytes in the last block.
        uint32_t bytesToCopy = (bytesRemaining > blockSize) ? blockSize : bytesRemaining;

        // Copy the data from the temporary block buffer to the main file buffer.
        memcpy(fileBuffer + (i * blockSize), blockBuffer, bytesToCopy);

        bytesRemaining -= bytesToCopy;
    }

    fileBuffer[fileSize] = '\0'; // Null-terminate the final buffer.

    kfree(blockBuffer); // Clean up the temporary buffer.
    return fileBuffer;  // Return the complete file data.
}

void extReadDrive(uint8_t drive)
{
    ext2_superblock_ext_t *superblock = extGetSuperblock(drive);
    ext2_blockgroupdescriptor_t *bgdt = extGetBGDT(drive, superblock);
    uint32_t blockSize = 1024 << superblock->logBlockSize;
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

    printf("Block Size: %i\n", blockSize);
    printf("BGDT Starting Block Addr of iNode Table: %i\n", bgdt->iNodeTableStartAddr);

    if (CEIL_DIV(superblock->totalBlocks, superblock->blocksPerGroup) != CEIL_DIV(superblock->totalINodes, superblock->iNodesPerGroup))
    {
        printf("EXT2: Error: Total blocks / Blocks per group != Total iNodes / iNodes\n");
    }
    ext2_inode_t *inode = extParseiNode(drive, superblock, bgdt, 2);
    extIterateDirectoryINode(drive, superblock, inode, dirEntryHasType);
    ext2_inode_t *textFileInode = extParseiNode(drive, superblock, bgdt, 13);

    if (textFileInode)
    {
        // Use the new function to read the file's contents
        uint8_t *fileContent = ext2ReadFile(drive, superblock, textFileInode);

        if (fileContent)
        {
            // If the read was successful, print the content.
            // We can cast to (char*) because we null-terminated it.
            printf("File Content (inode 13):\n---\n%s\n---\n", (char *)fileContent);

            // IMPORTANT: Free the buffer that ext2ReadFile allocated for us.
            kfree(fileContent);
        }
    }

    kfree(textFileInode);
    kfree(inode);
    kfree(bgdt);
    kfree(superblock);
}
