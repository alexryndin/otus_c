#include "../src/dbg.h"
#include "../src/hashmap_utils.h"
#include "../src/open_hashmap.h"
#include <stdio.h>

const char *const USAGE = "Usage: %s file [keys|values]\n";

enum { KEYS = 1, VALUES = 2 };

static int free_node(OpenHashmapNode *node) {
    gs_destroy(node->key);
    free(node->data);
    return 0;
}

static int sort_gstr(const OpenHashmapNode *a, const OpenHashmapNode *b) {
    if (IS_VACANT(a) && IS_VACANT(b))
        return 0;
    if (IS_VACANT(a) && !IS_VACANT(b))
        return -1;
    if (!IS_VACANT(a) && IS_VACANT(b))
        return 1;
    return gstr_cmp(a->key, b->key);
}

static int sort_vals(const OpenHashmapNode *a, const OpenHashmapNode *b) {
    if (IS_VACANT(a) && IS_VACANT(b))
        return 0;
    if (IS_VACANT(a) && !IS_VACANT(b))
        return -1;
    if (!IS_VACANT(a) && IS_VACANT(b))
        return 1;
    return uint32_cmp(a->data, b->data);
}

static int print_map(OpenHashmapNode *node) {
    CHECK(node != NULL, "null node.");
    if (((GS *)node->key)->len == 0) {
        printf("<whitespace>: %u\n", *(uint32_t *)node->data);

    } else {
        printf("%s: %u\n", ((GS *)node->key)->s, *(uint32_t *)node->data);
    }
    return 0;
error:
    return -1;
}

int main(int argc, char *argv[]) {
    int rc = 0;
    int sort_mode = 0;
    OpenHashmap *map = NULL;

    GS *b = gs_create();
    /*
     What if uint32_t will overflow...
    ⠀⠀⠀⠀⠀⢀⣀⣀⣀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
    ⠀⠀⠀⠰⡿⠿⠛⠛⠻⠿⣷⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
    ⠀⠀⠀⠀⠀⠀⣀⣄⡀⠀⠀⠀⠀⢀⣀⣀⣤⣄⣀⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀
    ⠀⠀⠀⠀⠀⢸⣿⣿⣷⠀⠀⠀⠀⠛⠛⣿⣿⣿⡛⠿⠷⠀⠀⠀⠀⠀⠀⠀⠀
    ⠀⠀⠀⠀⠀⠘⠿⠿⠋⠀⠀⠀⠀⠀⠀⣿⣿⣿⠇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
    ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠉⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
    ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
    ⠀⠀⠀⠀⣿⣷⣄⠀⢶⣶⣷⣶⣶⣤⣀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
    ⠀⠀⠀⠀⣿⣿⣿⠀⠀⠀⠀⠀⠈⠙⠻⠗⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
    ⠀⠀⠀⣰⣿⣿⣿⠀⠀⠀⠀⢀⣀⣠⣤⣴⣶⡄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
    ⠀⣠⣾⣿⣿⣿⣥⣶⣶⣿⣿⣿⣿⣿⠿⠿⠛⠃⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
    ⢰⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
    ⢸⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
    ⠈⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
    ⠀⠀⠛⢿⣿⣿⣿⣿⣿⣿⡿⠟⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
    ⠀⠀⠀⠀⠀⠉⠉⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
    */
    uint32_t *value = NULL;
    FILE *f = NULL;

    if (argc == 1) {
        printf(USAGE, argv[0]);
        goto exit;
    }
    if (argc == 3) {
        if (!strcmp(argv[2], "keys")) {
            sort_mode = KEYS;
        } else if (!strcmp(argv[2], "values")) {
            sort_mode = VALUES;
        } else {
            printf(USAGE, argv[0]);
            goto exit;
        }
    }

    f = fopen(argv[1], "r");
    CHECK(f != NULL, "Failed openning file %s", argv[1]);

    int ch = 0;

    map = OpenHashmap_create((OpenHashmap_compare)gstr_cmp,
                             (OpenHashmap_hash)gstr_hash);
    while ((ch = fgetc(f)) != EOF) {
        if (isspace(ch) || ispunct(ch)) {
            value = OpenHashmap_get(map, b);
            if (value != NULL) {
                gs_destroy(b);
                b = NULL;
                (*value)++;
            } else {
                value = calloc(1, sizeof(uint32_t));
                CHECK_MEM(value);
                *value = 1;
                rc = OpenHashmap_set(map, b, value);
                CHECK(rc == 0, "map set failed.");
            }
            b = gs_create();
        } else {
            gsconchar(b, ch);
        }
    }
    CHECK(errno == 0, "Error reading file.");

    switch (sort_mode) {
    case KEYS:
        qsort(map->buckets->contents, map->number_of_buckets,
              sizeof(OpenHashmapNode),
              (int (*)(const void *, const void *))sort_gstr);
        break;
    case VALUES:
        qsort(map->buckets->contents, map->number_of_buckets,
              sizeof(OpenHashmapNode),
              (int (*)(const void *, const void *))sort_vals);
        break;
    default:
        break;
    }

    OpenHashmap_traverse(map, print_map);
    OpenHashmap_traverse(map, free_node);

exit:
    if (f != NULL) {
        fclose(f);
        f = NULL;
    }
    if (b != NULL) {
        gs_destroy(b);
        b = NULL;
    }
    if (map != NULL) {
        OpenHashmap_destroy(map);
    }
    return rc;
error:
    rc = 1;
    goto exit;
}
