#include <stdio.h>
#include <string.h>
#include "main.h"
#include "dbg.h"

const char* const USAGE = "Usage: %s input output koi8|win1251|iso8859 \n";

int main(int argc, char *argv[]) {
    int rc = 0;
    if (argc != 4) {
        printf(USAGE, argv[0]);
        return 0;
    }

    const unsigned char (*encoding_table)[128][4] = NULL;

    if (!strcmp(argv[3], "koi8")) {
        encoding_table = &k8;
        LOG_DEBUG("Encoding k8");
    } else if (!strcmp(argv[3], "win1251")) {
        encoding_table = &w1251;
        LOG_DEBUG("Encoding w1251");
    } else if (!strcmp(argv[3], "iso8859")) {
        encoding_table = &iso;
        LOG_DEBUG("Encoding iso");
    } else {
        printf("Unsupported encoding %s\n", argv[3]);
        return 0;
    }

    FILE *input = fopen(argv[1], "r");
    FILE *output = fopen(argv[2], "w");

    CHECK(input != NULL, "Error opening input file for reading.");
    CHECK(output != NULL, "Error opening output file for writing.");

    int ch = 0;
    while ((ch = getc(input)) != EOF) {
        if (ch < 128) {
            rc = fputc(ch, output);
            CHECK(rc == ch, "write error.");
        } else {
            ch -= 128;
            for (int i = 0; i < MAX_CONV_LEN && (*encoding_table)[ch][i]; i++) {
                rc = fputc((*encoding_table)[ch][i], output);
                CHECK(rc == (*encoding_table)[ch][i], "write error.");
            }
        }
    }

    rc = 0; // fallthrough
exit:
    if (input) { fclose(input); input = 0; }
    if (output) { fclose(output); output = 0; }
    return rc;
error:
    rc = 1;
    goto exit;
}
