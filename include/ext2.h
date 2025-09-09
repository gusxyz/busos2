#ifndef EXT2_H
#define EXT2_H

// flags
#define EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER 0x0001
#define EXT2_FEATURE_RO_COMPAT_LARGE_FILE 0x0002
#define EXT2_FEATURE_RO_COMPAT_BTREE_DIR 0x0004
#define EXT4_FEATURE_RO_COMPAT_HUGE_FILE 0x0008
#define EXT4_FEATURE_RO_COMPAT_GDT_CSUM 0x0010
#define EXT4_FEATURE_RO_COMPAT_DIR_NLINK 0x0020
#define EXT4_FEATURE_RO_COMPAT_EXTRA_ISIZE 0x0040

#define REQ_COMPRESSION 0x0001
#define REQ_DIR_TYPE_FIELD 0x0002
#define REQ_JOURNAL_REPLAY 0x0004
#define REQ_JOURNAL_DEVICE 0x0008

#include <stdint.h>

extern enum ext2_enums_superblock_s {
    SUPERBLOCK_SIZE = 1024,
    SUPERBLOCK_LOCATION = 1024,
    SUPERBLOCK_SECTOR_LENGTH = 2,
} ext2_enums_superblock;

typedef enum ext2_enums_inodetype_s
{
    iFIFO = 0x1,
    iCHAR_DEV = 0x2,
    iDIR = 0x4,
    iBLOCK_DEV = 0X6,
    iFILE = 0X8,
    iSYMLINK = 0xA,
    iUNIX_SOCKET = 0xC,
} ext2_enums_inodetype_t;

extern enum ext2_enums_filesystemstate_s {
    CLEAN = 1,
    ERRORS = 2,
} ext2_enums_filesystemstate;

typedef enum ext2_enums_dirtype_s
{
    UNKNOWN,
    FILE,
    DIR,
    CHAR_DEV,
    BLOCK_DEV,
    FIFO,
    SOCKET,
    SYMLINK,
} ext2_enum_dirtype_t;

typedef struct ext2_superblock_s
{
    uint32_t totalINodes;
    uint32_t totalBlocks;
    uint32_t reservedBlocksForSuperUser;
    uint32_t totalUnllocatedBlocks;
    uint32_t totalUnllocatedINodes;
    uint32_t superBlockNumber;
    uint32_t logBlockSize;
    uint32_t logFramgentSize;
    uint32_t blocksPerGroup;
    uint32_t fragsPerGroup;
    uint32_t iNodesPerGroup;
    uint32_t lastMountTime;
    uint32_t lastWrittenTime;
    uint16_t timesSinceLastFSCK;
    uint16_t mountsAllowedBeforeFSCK;
    uint16_t signature;
    uint16_t fileSystemState;
    uint16_t handleError;
    uint16_t minorPortionVersion;
    uint32_t lastFSCKTime;
    uint32_t intervalBetweenFSCK;
    uint32_t OSIDCreator;
    uint32_t majorPortionVersion;
    uint16_t userIdCanUseReserved;
    uint16_t groupIdCanUseReserved;
} __attribute__((packed)) ext2_superblock_t;

typedef struct ext2_superblock_s_ext
{
    uint32_t totalINodes;
    uint32_t totalBlocks;
    uint32_t reservedBlocksForSuperUser;
    uint32_t totalUnllocatedBlocks;
    uint32_t totalUnllocatedINodes;
    uint32_t superBlockNumber;
    uint32_t logBlockSize;
    uint32_t logFramgentSize;
    uint32_t blocksPerGroup;
    uint32_t fragsPerGroup;
    uint32_t iNodesPerGroup;
    uint32_t lastMountTime;
    uint32_t lastWrittenTime;
    uint16_t timesSinceLastFSCK;
    uint16_t mountsAllowedBeforeFSCK;
    uint16_t signature;
    uint16_t fileSystemState;
    uint16_t handleError;
    uint16_t minorPortionVersion;
    uint32_t lastFSCKTime;
    uint32_t intervalBetweenFSCK;
    uint32_t OSIDCreator;
    uint32_t majorPortionVersion;
    uint16_t userIdCanUseReserved;
    uint16_t groupIdCanUseReserved;

    // extension
    uint32_t firsNonReservediNode;
    uint16_t iNodeSize;
    uint16_t blockGroupMember; // Block group that this superblock is part of (if backup copy)
    uint32_t optionalFlags;
    uint32_t requiredFlags;
    uint32_t readOnlyFlags;
    uint8_t fileSystemId[16];
    uint8_t volumeName[16];
} __attribute__((packed)) ext2_superblock_ext_t;

typedef struct ext2_blockgroupdescriptor_s
{
    uint32_t blockUsageBitmapAddr;
    uint32_t iNodeUsageBitmapAddr;
    uint32_t iNodeTableStartAddr;
    uint16_t numUnllocatedBlocks;
    uint16_t numUnllocatedINodes;
    uint16_t numDirs;
} __attribute__((packed)) ext2_blockgroupdescriptor_t;

typedef struct ext2_inode_s
{
    uint16_t typePerm;
    uint16_t userId;
    uint32_t size_lo;
    uint32_t plastAccessTime;
    uint32_t pCreationTime;
    uint32_t pLastModTime;
    uint32_t pDeletionTime;
    uint16_t groupId;
    uint16_t hardLinkCount;
    uint32_t countDiskSectors;
    uint32_t flags;
    uint32_t osSpecificValue;
    uint32_t dbPtrs[12];
    uint32_t singleIndirectBlckPtr;
    uint32_t doubleIndirectBlckPtr;
    uint32_t tripleIndirectBlckPtr;
    uint32_t genNumber;
    uint32_t extendedAttributeBLock;
    uint32_t size_hi;
    uint32_t blockAddrFrag;
} __attribute__((packed)) ext2_inode_t;

typedef struct ext2_dir_s
{
    uint32_t iNode;
    uint16_t totalSize;
    uint8_t nameLengthLSB;
    uint8_t typeOrNameLengthMSB;
    char nameCharacters[];
} __attribute__((aligned(4))) ext2_dir_t;

#endif