#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "dbg.h"

const char * const USAGE = "Usage: %s [-o new_filename] filename\n";

unsigned char JPG_SOI[] = {0xFF, 0xD8};
unsigned char JPG_EOI[] = {0xFF, 0xD9};
unsigned char ZIP_SIG[] = {0x50, 0x4B, 0x03, 0x04};

struct ZipHeader {
    char flag3;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint16_t filename_len;
    uint16_t extrafield_len;
};

// Check file into *preallocated* ZipHeader structure passed by reference
int read_zip_header(struct ZipHeader *zh, FILE *f) {
    int rc = 0;
    uint16_t flags = 0;

    // skip data we're not interested in...
    fseek(f, 2, SEEK_CUR);
    rc = fread(&flags, sizeof(uint16_t), 1, f);
    fseek(f, 10, SEEK_CUR);

    rc += fread(&(zh)->compressed_size, sizeof(uint32_t), 1, f);
    rc += fread(&(zh)->uncompressed_size, sizeof(uint32_t), 1, f);
    rc += fread(&(zh)->filename_len, sizeof(uint16_t), 1, f);
    rc += fread(&(zh)->extrafield_len, sizeof(uint16_t), 1, f);
    (zh)->flag3 = flags & 4;

    CHECK(rc == 5 && !feof(f), "read_zip_header error, rc = %d, eof = %d.", rc, feof(f));

    LOG_DEBUG("Zip header info -- filename size is %d, compressed_size = %d, extrafield_len = %d, flag3 = %d", zh->filename_len, zh->compressed_size, zh->extrafield_len, zh->flag3);

    return 0;

error:
    return -1;
}

// Check current file position for zip signature bytes
int check_zip_sig(unsigned char *buf, FILE *f) {
    size_t rc = 0;
    rc = fread(buf, sizeof(unsigned char), sizeof(ZIP_SIG), f);
    CHECK(errno == 0, "File reading error.");
    if((rc < sizeof(ZIP_SIG)) && feof(f)) {
        LOG_DEBUG("eof");
        return 0;
    }
    if((rc < sizeof(ZIP_SIG)) && !feof(f)) {
        SENTINEL("File reading error, panic!");
    }

    rc = memcmp(buf, ZIP_SIG, sizeof(ZIP_SIG));
    if (rc != 0) {
        return 0;
    } else {
        return 1;
    }

error:
    return -1;
}


int main(int argc, char *argv[]) {
    unsigned char lil_buf[6] = { 0 }; // 6 is enough :)
    char *filename = NULL;
    char *zip_filename = NULL;
    char *newfilename = NULL;
    FILE *f = NULL, *nf = NULL;

    struct ZipHeader *zh = malloc(sizeof(struct ZipHeader));
    CHECK_MEM(zh);

    long int rc = 0, zip_start = 0;

    switch (argc) {
    case 4:
        if (!strcmp(argv[1], "-o")) {
            newfilename = argv[2];
            zip_filename = argv[3];
        } else {
            printf(USAGE, argv[0]);
            return 0;
        }
        break;
    case 2:
        zip_filename = argv[1];
        break;
    default:
        printf(USAGE, argv[0]);
        return 0;
    }

    f = fopen(zip_filename, "r");
    CHECK(f != NULL, "Error opening file %s", zip_filename);

    rc = fread(lil_buf, sizeof(unsigned char), sizeof(JPG_SOI), f);
    CHECK(rc == sizeof(JPG_SOI), "File reading error.");

    rc = memcmp(lil_buf, JPG_SOI, sizeof(JPG_SOI));
    if (rc != 0) {
        printf("Seems like your file is not a jpeg\n");
        return 0;
    }

    rc = fread(lil_buf, sizeof(unsigned char), sizeof(JPG_EOI), f);
    CHECK(rc == sizeof(JPG_EOI), "File reading error.");
    rc = memcmp(lil_buf, JPG_EOI, sizeof(JPG_EOI));
    if (rc == 0) {
        LOG_DEBUG("jpg EOF reached");
    } else {
        while (rc) {
            // Thankfully jpg markers are only two bytes sized,
            // so we don't need to use memcpy and other stuff
            lil_buf[0] = lil_buf[1];
            rc = fread(lil_buf+1, sizeof(unsigned char), 1, f);
            CHECK(rc == 1, "File reading error.");
            rc = memcmp(lil_buf, JPG_EOI, sizeof(JPG_EOI));
        }
    }
    rc = ftell(f);
    CHECK(rc > 0, "ftell failed");
    zip_start = rc;
    LOG_DEBUG("jpeg eof pos = %lx", zip_start);

    if (newfilename) {
        nf = fopen(newfilename, "w");
        CHECK(nf != NULL, "Error opening file %s", newfilename);
        int c = 0;
        while ((c = fgetc(f)) != EOF) {
            CHECK(fputc(c, nf) == c, "Error writing to a new file.");
        }
        fclose(nf); nf = NULL;
        CHECK(errno == 0, "Error writing to a new file.");
        rc = fseek(f, zip_start, SEEK_SET);
        CHECK(rc == 0, "Error rewinding to the start of zip-file.");
    }

    // Check zip header signature
    rc = check_zip_sig(lil_buf, f);
    CHECK(rc >= 0, "Error checking zip signature.");
    if (!rc) {
        printf("Seems like your file is not a jpeg-zip\n");
        return 0;
    }

    LOG_DEBUG("jpeg-zip found");

    uint16_t filename_len = 2;
    filename = calloc(filename_len, sizeof(char));
    char end = 0;

    while (!end) {
        rc = read_zip_header(zh, f);
        CHECK(rc == 0, "Failed to read ZipHeader.");

        if (zh->filename_len+1 > filename_len) {
            filename = realloc(filename, zh->filename_len + 1);
            filename[zh->filename_len] = '\0';
            filename_len = zh->filename_len;
        }

        rc = fread(filename, sizeof(char), zh->filename_len, f);
        filename[zh->filename_len] = '\0';
        CHECK(errno == 0 && rc == zh->filename_len, "File reading error.");
        printf("%s\n", filename);

        fseek(f, zh->extrafield_len, SEEK_CUR);
        fseek(f, zh->compressed_size, SEEK_CUR);
        // skip data descriptor if present.
        if (zh->flag3) {
            fseek(f, 12, SEEK_CUR);
        }
        LOG_DEBUG("Current pos = %lx", ftell(f));
        rc = check_zip_sig(lil_buf, f);
        CHECK(rc >= 0, "Error checking zip signature.");
        if (!rc) {
            end = 1;
        }
    }


    rc = 0;
exit:
    // resources cleanup
    if (f) { fclose(f); f = NULL; }
    if (nf) { fclose(nf); nf = NULL; }
    if (zh) { free(zh); zh = NULL; }
    if (filename) { free(filename); filename = NULL; }
    return rc;
error:
    rc = 1;
    goto exit;
}
