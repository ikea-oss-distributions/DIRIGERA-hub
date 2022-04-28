// Copyright © Inter IKEA Systems B.V. 2017, 2018, 2019, 2020, 2021.
// All Rights Reserved.
//
// This is UNPUBLISHED PROPRIETARY SOURCE CODE of © Inter IKEA Systems B.V.;
// the contents of this file may not be disclosed to third parties, copied
// or duplicated in any form, in whole or in part, without the prior
// written permission of © Inter IKEA Systems B.V.

#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

////////////////////////////////////////////////////////////
//  Start http://home.thep.lu.se/~bjorn/crc/crc32_fast.c  //
////////////////////////////////////////////////////////////

uint32_t crc32_for_byte(uint32_t r)
{
    for (int j = 0; j < 8; ++j)
    {
        r = (r & 1 ? 0 : (uint32_t)0xEDB88320L) ^ r >> 1;
    }
    return r ^ (uint32_t)0xFF000000L;
}

/* Any unsigned integer type with at least 32 bits may be used as
 * accumulator type for fast crc32-calulation, but unsigned long is
 * probably the optimal choice for most systems. */
typedef unsigned long accum_t;

void init_tables(uint32_t* table, uint32_t* wtable)
{
    for (size_t i = 0; i < 0x100; ++i)
    {
        table[i] = crc32_for_byte(i);
    }
    for (size_t k = 0; k < sizeof(accum_t); ++k)
        for (size_t w, i = 0; i < 0x100; ++i)
        {
            for (size_t j = w = 0; j < sizeof(accum_t); ++j)
            {
                w = table[(uint8_t)(j == k ? w^ i : w)] ^ w >> 8;
            }
            wtable[(k << 8) + i] = w ^ (k ? wtable[0] : 0);
        }
}

void crc32(const void* data, size_t n_bytes, uint32_t* crc)
{
    static uint32_t table[0x100], wtable[0x100 * sizeof(accum_t)];
    size_t n_accum = n_bytes / sizeof(accum_t);
    if (!*table)
    {
        init_tables(table, wtable);
    }
    for (size_t i = 0; i < n_accum; ++i)
    {
        accum_t a = *crc ^ ((accum_t*)data)[i];
        for (size_t j = *crc = 0; j < sizeof(accum_t); ++j)
        {
            *crc ^= wtable[(j << 8) + (uint8_t)(a >> 8 * j)];
        }
    }
    for (size_t i = n_accum * sizeof(accum_t); i < n_bytes; ++i)
    {
        *crc = table[(uint8_t) * crc ^ ((uint8_t*)data)[i]] ^ *crc >> 8;
    }
}

//////////////////////////////////////////////////////////
//  End http://home.thep.lu.se/~bjorn/crc/crc32_fast.c  //
//////////////////////////////////////////////////////////

// Convert string to byte, e.g. "B4" -> 0xB4
static uint8_t to_byte(const char* uuid_byte)
{
    uint8_t val = ((uuid_byte[0] <= 0x39) ? uuid_byte[0] - 0x30 : uuid_byte[0] - 0x57) << 4;
    val += (uuid_byte[1] <= 0x39) ? uuid_byte[1] - 0x30 : uuid_byte[1] - 0x57;
    return val;
}

// Convert e.g. ebd0a0a2-b9e5-4433-87c0-68b6b72699c7 into 16 bytes suitable for metadata
static void uuid_to_bytes(const char* uuid, uint8_t* bytes)
{
    for (int i = 0; i < 4; i++)
    {
        bytes[i] = to_byte(uuid + 6 - (i * 2));
    }

    bytes[4] = to_byte(uuid + 11);
    bytes[5] = to_byte(uuid + 9);

    bytes[6] = to_byte(uuid + 16);
    bytes[7] = to_byte(uuid + 14);

    bytes[8] = to_byte(uuid + 19);
    bytes[9] = to_byte(uuid + 21);

    for (int i = 10; i < 16; i++)
    {
        bytes[i] = to_byte(uuid + 34 - (i - 10) * 2);
    }
}

// Partitions to write the metadata to
static char* metadata_partitions[2] =
{
    "/dev/disk/by-partlabel/teed",
    "/dev/disk/by-partlabel/teex"
};

static const uint8_t UUID_LEN = 16;
static const uint8_t METADATA_LEN = 96;

static const uint16_t MMC_UUID_OFFSET = (512 + 56);

static const char type_uuid[] = "ebd0a0a2-b9e5-4433-87c0-68b6b72699c7";

// Get the mmc uuid from the GPT header
static int get_mmc_uuid(uint8_t* mmc_uuid_gpt)
{
    int ret = 0;

    FILE* fp = fopen("/dev/mmcblk1", "rb");
    if (fp != NULL)
    {
        ret = fseek(fp, MMC_UUID_OFFSET, SEEK_SET);
        if (ret == 0)
        {
            size_t bytes_read = fread(mmc_uuid_gpt, 1, UUID_LEN, fp);
            if (bytes_read == UUID_LEN)
            {
                goto exit;
            }
            else
            {
                printf("Failed to read in mmc. Bytes read: %u, errno: %d\n", bytes_read, errno);
                ret = -2;
                goto exit;
            }
        }
        else
        {
            printf("Failed to seek in mmc: %d\n", ret);
            ret = -3;
            goto exit;
        }

    }
    else
    {
        printf("Failed to open mmc device '/dev/mmcblk1'.\n");
        return -1;
    }

exit:
    fclose(fp);
    return ret;
}


static int get_partition(char* name, char* partition, size_t len)
{
    struct dirent* dir;
    bool found = false;
    int ret = 0;

    DIR* by_partlabel = opendir("/dev/disk/by-partlabel");
    if (by_partlabel)
    {
        chdir("/dev/disk/by-partlabel");
        while ((dir = readdir(by_partlabel)))
        {
            if (strcmp(name, dir->d_name) == 0)
            {
                ssize_t length = readlink(dir->d_name, partition, len);
                if (length != -1)
                {
                    partition[length < len ? length : len - 1] = '\0';
                    printf("%s -> %s\n", name, partition);
                    found = true;
                }
                else
                {
                    printf("Failed reading link %s\n", dir->d_name);
                    ret = -1;
                }
                break;
            }
        }
    }
    else
    {
        printf("Failed to open by-partlabel dir.\n");
        ret = -2;
    }
    closedir(by_partlabel);

    if (!found)
    {
        ret = -3;
    }

    return ret;
}

static int get_partition_uuid(char* partition, uint8_t* uuid)
{
    struct dirent* dir;
    bool found = false;
    int ret = 0;

    DIR* by_partuuid = opendir("/dev/disk/by-partuuid");
    if (by_partuuid)
    {
        chdir("/dev/disk/by-partuuid");
        while ((dir = readdir(by_partuuid)))
        {
            char uuid_link[128];
            if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
            {
                continue;
            }
            ssize_t length = readlink(dir->d_name, uuid_link, sizeof(uuid_link));
            if (length != -1)
            {
                uuid_link[length < sizeof(uuid_link) ? length : sizeof(uuid_link) - 1] = '\0';
                if (strcmp(partition, uuid_link) == 0)
                {
                    printf("Found uuid for %s: %s\n", partition, dir->d_name);
                    uuid_to_bytes(dir->d_name, uuid);
                    found = true;
                    break;
                }
            }
            else
            {
                printf("Failed to read link for: %s\n", dir->d_name);
                ret = -1;
            }
        }
    }
    else
    {
        printf("Failed to open by-partuuid dir.\n");
        ret = -2;
    }
    closedir(by_partuuid);

    if (!found)
    {
        ret = -3;
    }

    return ret;
}


// Get partition uuid from /dev/disk/by-partlabel & by-partuuid
// Convert it to bytes suitable for metadata and store it in uuid input.
static int get_partlabel_uuid(char* partlabel, uint8_t* uuid)
{
    char partition[128] = {0};
    int ret = 0;

    ret = get_partition(partlabel, partition, sizeof(partition));

    if (ret == 0)
    {
        ret = get_partition_uuid(partition, uuid);
        if (ret != 0)
        {
            printf("Couldn't find uuid for '%s'.\n", partition);
        }
    }
    else
    {
        printf("Couldn't find partlabel '%s'.\n", partlabel);
    }

    return ret;
}

int main(int argc, char** argv)
{
    int ret = 0;
    uint32_t active_index = 0;
    uint32_t previous_index = 1;

    uint32_t crc = {0};
    uint32_t version = 0;
    uint32_t accepted = 1;

    uint8_t metadata[METADATA_LEN];

    uint8_t mmc_bytes[UUID_LEN];
    uint8_t type_bytes[UUID_LEN];
    uint8_t ssbl1_bytes[UUID_LEN];
    uint8_t ssbl2_bytes[UUID_LEN];

    if (argc != 2)
    {
        printf("Bad args.\n");
        printf("Usage: %s <next slot (A/B)>\n", argv[0]);
        return -1;
    }

    if (strlen(argv[1]) != 1)
    {
        printf("Next active index must be A or B.");
        return -2;
    }

    if (argv[1][0] == 'A')
    {
        active_index = 0;
        previous_index = 1;
    }
    else if (argv[1][0] == 'B')
    {
        active_index = 1;
        previous_index = 0;
    }
    else
    {
        printf("Next active index must be A or B.");
        return -3;
    }

    uint8_t mmc_gpt_uuid[UUID_LEN];
    ret = get_mmc_uuid(mmc_gpt_uuid);

    // Reorder bytes for metadata
    memcpy(mmc_bytes, mmc_gpt_uuid, 10);
    for (int i = 10; i < UUID_LEN; i++)
    {
        uint8_t val = mmc_gpt_uuid[15 - (i - 10)];
        mmc_bytes[i] = val;
    }

    ret = get_partlabel_uuid("ssbl1", ssbl1_bytes);
    if (ret != 0)
    {
        printf("Failed to find uuid for ssbl1\n");
        return -4;
    }

    ret = get_partlabel_uuid("ssbl2", ssbl2_bytes);
    if (ret != 0)
    {
        printf("Failed to find uuid for ssbl2\n");
        return -5;
    }

    uuid_to_bytes(type_uuid, type_bytes);

    // Copy the different parts into the metadata blob
    uint8_t pos = 4; // CRC32 in first 4 bytes
    memcpy(metadata + pos, &version, sizeof(version));
    pos += sizeof(version);
    memcpy(metadata + pos, &active_index, sizeof(active_index));
    pos += sizeof(active_index);
    memcpy(metadata + pos, &previous_index, sizeof(previous_index));
    pos += sizeof(previous_index);
    memcpy(metadata + pos, type_bytes, sizeof(type_bytes));
    pos += sizeof(type_bytes);
    memcpy(metadata + pos, mmc_bytes, sizeof(mmc_bytes));
    pos += sizeof(mmc_bytes);
    memcpy(metadata + pos, ssbl1_bytes, sizeof(ssbl1_bytes));
    pos += sizeof(ssbl1_bytes);
    memcpy(metadata + pos, &accepted, sizeof(accepted));
    pos += sizeof(accepted);
    memset(metadata + pos, 0x00, 4); // Reserved, must be zero
    pos += 4;
    memcpy(metadata + pos, ssbl2_bytes, sizeof(ssbl2_bytes));
    pos += sizeof(ssbl2_bytes);
    memcpy(metadata + pos, &accepted, sizeof(accepted));
    pos += sizeof(accepted);
    memset(metadata + pos, 0x00, 4); // Reserved, must be zero
    pos += 4;

    crc32(metadata + 4, pos - 4, &crc);
    memcpy(metadata, &crc, sizeof(crc));

    // Write the metadata to metadata partitions.
    for (int i = 0; i < (sizeof(metadata_partitions) / sizeof(metadata_partitions[0])); i++)
    {
        char metadata_part[128] = {0};
        snprintf(metadata_part, sizeof(metadata_part), "%s%u", metadata_partitions[i], active_index + 1);
        printf("Writing metadata to %s\n", metadata_part);
        FILE* fp = fopen(metadata_part, "wb");
        if (fp)
        {
            size_t bytes_written = fwrite(metadata, 1, pos, fp);
            if (bytes_written != pos)
            {
                printf("fwrite wrote %u bytes instead of %u as expected to %s\n", bytes_written, pos, argv[i]);
                fclose(fp);
                return -6;
            }
            fclose(fp);
        }
        else
        {
            printf("fopen returned NULL for %s\n", argv[i]);
            return -7;
        }

    }

    printf("Done!\n");
    return 0;
}
