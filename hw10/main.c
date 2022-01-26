#include "dbg.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

extern const uint32_t CRCTable[256];

#define USAGE "Usage: %s filepath block_size [just_read|mmap]"

int just_read_crc32(size_t size, char *filepath, uint32_t *crc32) {
    int rc = 0;
    FILE *f = NULL;
    size_t filesize;
    size_t to_read = 0;
    size_t watermark = 0;
    struct stat filestat;
    long page_size = sysconf(_SC_PAGE_SIZE);

    CHECK(crc32 != NULL, "null pointer");
    LOG_DEBUG("page size is %ld", page_size);
    CHECK(size > 0 && size < 32768, "Wrong block size");

    /* round block_size to page_size (4k in Linux) */
    watermark = ((size_t)(size * 1024. * 1024. / page_size)) * page_size;
    LOG_DEBUG("watermark is %ld", watermark);

    CHECK(lstat(filepath, &filestat) == 0, "Couldn't get filestat");
    CHECK((f = fopen(filepath, "r")) != NULL, "Couldn't open file");

    filesize = (size_t)filestat.st_size;
    LOG_DEBUG("filesize = %zu", filesize);

    void *ptr = malloc(watermark);
    CHECK_MEM(ptr);
    CHECK(fread(ptr, 1, watermark, f) > 0, "Couldn't read file");

    *crc32 = 0xFFFFFFFFu;
    for (size_t i = 0; i < filesize; i++, to_read++) {
        if (to_read >= watermark) {
            CHECK(fread(ptr, 1, watermark, f) > 0, "Couldn't read file");
            LOG_DEBUG("watermark reached");
            to_read = 0;
        }
        const uint32_t lookupIndex =
            (*crc32 ^ ((uint8_t *)ptr)[to_read]) & 0xff;
        *crc32 =
            (*crc32 >> 8) ^ CRCTable[lookupIndex]; // CRCTable is an array of
                                                   // 256 32-bit constants
    }

    // Finalize the CRC-32 value by inverting all the bits
    *crc32 ^= 0xFFFFFFFFu;

    printf("%x\n", *crc32);
exit:
    if (f != NULL)
        fclose(f);
    return rc;
error:
    rc = -1;
    goto exit;
}

int mmap_crc32(size_t size, char *filepath, uint32_t *crc32) {
    int rc = 0;
    int fd = 0;
    size_t filesize;
    size_t to_discard = 0;
    size_t watermark = 0;
    struct stat filestat;
    long page_size = sysconf(_SC_PAGE_SIZE);

    CHECK(crc32 != NULL, "null pointer");
    LOG_DEBUG("page size is %ld", page_size);
    CHECK(size > 0 && size < 32768, "Wrong block size");

    /* round block_size to page_size (4k in Linux) */
    watermark = ((size_t)(size * 1024. * 1024. / page_size)) * page_size;
    to_discard = watermark;
    LOG_DEBUG("watermark is %ld", watermark);

    CHECK(lstat(filepath, &filestat) == 0, "Couldn't get filestat");
    CHECK((fd = open(filepath, O_RDONLY)) > 0, "Couldn't open file");

    filesize = (size_t)filestat.st_size;

    void *ptr = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, fd, 0);
    CHECK_MEM(ptr);

    *crc32 = 0xFFFFFFFFu;

    for (size_t i = 0; i < filesize; i++) {
        const uint32_t lookupIndex = (*crc32 ^ ((uint8_t *)ptr)[i]) & 0xff;
        *crc32 =
            (*crc32 >> 8) ^ CRCTable[lookupIndex]; // CRCTable is an array of
                                                   // 256 32-bit constants
        if (i >= to_discard) {
            // CHECK(munmap(ptr, to_discard) == 0, "Couldn't unmap memory block");
            to_discard += watermark;
        }
    }

    // Finalize the CRC-32 value by inverting all the bits
    *crc32 ^= 0xFFFFFFFFu;

     // CHECK(munmap(ptr, 4096) == 0, "Couldn't unmap");

    printf("%x\n", *crc32);
exit:
    if (fd > 0)
        close(fd);
    return rc;
error:
    rc = -1;
    goto exit;
}

int main(int argc, char *argv[]) {
    uint32_t crc32;

    if (argc != 3 && argc != 4)
        goto usage;

    int block_size = atoi(argv[2]);
    CHECK(block_size > 0 && block_size < 32768, "Wrong block size");

    if (argc == 4) {
        if (!strcmp(argv[3], "just_read")) {
            just_read_crc32(block_size, argv[1], &crc32);
        } else if (!strcmp(argv[3], "mmap")) {
            mmap_crc32(block_size, argv[1], &crc32);
        } else {
            goto usage;
        }
    } else {
        mmap_crc32(block_size, argv[1], &crc32);
    }

    return 0;
usage:
    printf(USAGE, argv[0]);
error:
    return -1;
}
