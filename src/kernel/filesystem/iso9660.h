#pragma once
#include <stdint.h>

typedef uint8_t iso711_t;
typedef int8_t iso712_t;
typedef uint16_t iso721_t;
typedef uint16_t iso722_t;
typedef uint32_t iso723_t;
typedef uint32_t iso731_t;
typedef uint32_t iso732_t;
typedef uint64_t iso733_t;
typedef char achar_t;
typedef char dchar_t;

#define ISO_MAX_PUBLISHER_ID 128

#define ISO_MAX_APPLICATION_ID 128

#define ISO_MAX_VOLUME_ID 32

#define ISO_MAX_VOLUMESET_ID 128

#ifdef ISODCL
#undef ISODCL
#endif

/* This part borrowed from the bsd386 isofs */
#define ISODCL(from, to) ((to) - (from) + 1)

extern enum iso_enum1_s {
    ISO_PVD_SECTOR = 16,
    ISO_EVD_SECTOR = 17,
    LEN_ISONAME = 31,
    ISO_MAX_SYSTEM_ID = 32,
    MAX_ISONAME = 37,
    ISO_MAX_PREPARER_ID = 128,
    MAX_ISOPATHNAME = 255,
    ISO_BLOCKSIZE = 2048
} iso_enums1;

extern enum iso_flag_enum_s {
    ISO_FILE = 0,
    ISO_EXISTENCE = 1,
    ISO_DIRECTORY = 2,
    ISO_ASSOCIATED = 4,
    ISO_RECORD = 8,
    ISO_PROTECTION = 16,
    ISO_DRESERVED1 = 32,
    ISO_DRESERVED2 = 64,
    ISO_MULTIEXTENT = 128,
} iso_flag_enums;

extern enum iso_vd_enum_s {
    ISO_VD_BOOT_RECORD = 0,
    ISO_VD_PRIMARY = 1,
    ISO_VD_SUPPLEMENTARY = 2,
    ISO_VD_PARITION = 3,
    ISO_VD_END = 255
} iso_vd_enums;

struct iso9660_dtime_s
{
    iso711_t dt_year;
    iso711_t dt_month;
    iso711_t dt_day;
    iso711_t dt_hour;
    iso711_t dt_minute;
    iso711_t dt_second;
    iso712_t dt_gmtoff;
} __attribute__((packed));
typedef struct iso9660_dtime_s iso9660_dtime_t;

struct iso9660_ltime_s
{
    char lt_year[ISODCL(1, 4)];
    char lt_month[ISODCL(5, 6)];
    char lt_day[ISODCL(7, 8)];
    char lt_hour[ISODCL(9, 10)];
    char lt_minute[ISODCL(11, 12)];
    char lt_second[ISODCL(13, 14)];
    char lt_hsecond[ISODCL(15, 16)];
    iso712_t lt_gmtoff;
} __attribute__((packed));

typedef struct iso9660_ltime_s iso9660_ltime_t;
typedef struct iso9660_dir_s iso9660_dir_t;
typedef struct iso9660_stat_s iso9660_stat_t;

struct iso9660_dir_s
{
    iso711_t length;
    iso711_t xa_length;
    iso733_t extent;
    iso733_t size;
    iso9660_dtime_t recording_time;
    uint8_t file_flags;
    iso711_t file_unit_size;
    iso711_t interleave_gap;
    iso723_t volume_sequence_number;
    union
    {
        iso711_t len;
        char str[1];
    } filename;
} __attribute__((packed));

struct iso9660_pvd_s
{
    iso711_t type;
    char id[5];
    iso711_t version;
    char unused1[1];
    achar_t system_id[ISO_MAX_SYSTEM_ID];
    dchar_t volume_id[ISO_MAX_VOLUME_ID];
    uint8_t unused2[8];
    iso733_t volume_space_size;
    uint8_t unused3[32];
    iso723_t volume_set_size;
    iso723_t volume_sequence_number;
    iso723_t logical_block_size;
    iso733_t pathTableSize;
    iso731_t type_l_path_table;
    iso731_t opt_type_l_path_table;
    iso732_t type_m_path_table;
    iso732_t opt_type_m_path_table;
    iso9660_dir_t root_directory_record;
    char root_directory_filename;
    dchar_t volume_set_id[ISO_MAX_VOLUMESET_ID];
    achar_t publisher_id[ISO_MAX_PUBLISHER_ID];
    achar_t preparer_id[ISO_MAX_PREPARER_ID];
    achar_t application_id[ISO_MAX_APPLICATION_ID];
    dchar_t copyright_file_id[37];
    dchar_t abstract_file_id[37];
    dchar_t bibliographic_file_id[37];
    iso9660_ltime_t creation_date;
    iso9660_ltime_t modification_date;
    iso9660_ltime_t expiration_date;
    iso9660_ltime_t effective_date;
    iso711_t file_structure_version;
    uint8_t unused4[1];
    char application_data[512];
    uint8_t unused5[653];
} __attribute__((packed));

typedef struct iso9660_pvd_s iso9660_pvd_t;

struct iso9660_svd_s
{
    iso711_t type;
    char id[5];
    iso711_t version;
    char flags;
    achar_t system_id[ISO_MAX_SYSTEM_ID];
    dchar_t volume_id[ISO_MAX_VOLUME_ID];
    char unused2[8];
    iso733_t volume_space_size;
    char escape_sequences[32];
    iso723_t volume_set_size;
    iso723_t volume_sequence_number;
    iso723_t logical_block_size;
    iso733_t path_table_size;
    iso731_t type_l_path_table;
    iso731_t opt_type_l_path_table;
    iso732_t type_m_path_table;
    iso732_t opt_type_m_path_table;
    iso9660_dir_t root_directory_record;
    char root_directory_filename;
    dchar_t volume_set_id[ISO_MAX_VOLUMESET_ID];
    achar_t publisher_id[ISO_MAX_PUBLISHER_ID];
    achar_t preparer_id[ISO_MAX_PREPARER_ID];
    achar_t application_id[ISO_MAX_APPLICATION_ID];
    dchar_t copyright_file_id[37];
    dchar_t abstract_file_id[37];
    dchar_t bibliographic_file_id[37];
    iso9660_ltime_t creation_date;
    iso9660_ltime_t modification_date;
    iso9660_ltime_t expiration_date;
    iso9660_ltime_t effective_date;
    iso711_t file_structure_version;
    uint8_t unused4[1];
    char application_data[512];
    uint8_t unused5[653];
} __attribute__((packed));

struct iso9660_pathtable_s
{
    uint8_t len_di;        // Length of Directory Identifier (filename)
    uint8_t xa_length;     // Extended Attribute Record length
    iso731_t extent;       // LBA of the directory's data content
    iso721_t parent_di_no; // 1-based index of the parent directory's record in THIS path table
    char identifier[1];    // The directory name itself (this is a variable-length field)
} __attribute__((packed));

typedef struct iso9660_pathtable_s iso9660_pathtable_t;
