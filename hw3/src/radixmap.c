/*
 * Based on code by Andre Rainald then heavily modified by Zed A. Shaw
 * And then even more modified by Alex Ryndin
 */

#include "radixmap.h"
#include "../src/dbg.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

RadixMap *RadixMap_create(size_t max) {
    RadixMap *map = calloc(1, sizeof(RadixMap));
    CHECK_MEM(map);

    map->contents = calloc(sizeof(RMElement), max + 1);
    CHECK_MEM(map->contents);

    map->temp = calloc(sizeof(RMElement), max + 1);
    CHECK_MEM(map->temp);

    map->max = max;
    map->end = 0;

    return map;
error:
    if (map->contents != NULL) free(map->contents);
    if (map->temp != NULL) free(map->temp);
    if (map != NULL) free(map);
    return NULL;
}

void RadixMap_destroy(RadixMap *map) {
    if (map) {
        free(map->contents);
        free(map->temp);
        free(map);
    }
}

#define ByteOf(x, y) (((uint8_t *)x)[(y)])

static inline void radix_sort(short offset, uint64_t max, uint64_t *source,
                              uint64_t *dest) {
    uint64_t count[256] = {0};
    uint64_t *cp = NULL;
    uint64_t *sp = NULL;
    uint64_t *end = NULL;
    uint64_t s = 0;
    uint64_t c = 0;

    // count occurences of every byte value
    for (sp = source, end = source + max; sp < end; sp++) {
        count[ByteOf(sp, offset)]++;
    }

    // transform count into index by summing
    // elements and storing into same array
    for (s = 0, cp = count, end = count + 256; cp < end; cp++) {
        c = *cp;
        *cp = s;
        s += c;
    }

    // fill dest with the right values in the right place
    for (sp = source, end = source + max; sp < end; sp++) {
        cp = count + ByteOf(sp, offset);
        dest[*cp] = *sp;
        ++(*cp);
    }
}

void RadixMap_sort(RadixMap *map) {
    uint64_t *source = &map->contents[0].raw;
    uint64_t *temp = &map->temp[0].raw;

    radix_sort(0, map->end, source, temp);
    radix_sort(1, map->end, temp, source);
    radix_sort(2, map->end, source, temp);
    radix_sort(3, map->end, temp, source);
}

RMElement *RadixMap_find(RadixMap *map, uint32_t to_find) {
    int low = 0;
    int high = map->end - 1;
    RMElement *data = map->contents;

    while (low <= high) {
        int middle = low + (high - low) / 2;
        uint32_t key = data[middle].data.key;

        if (to_find < key) {
            high = middle - 1;
        } else if (to_find >key) {
            low = middle + 1;
        } else {
            return &data[middle];
        }
    }

    return NULL;
}

int RadixMap_add(RadixMap *map, uint32_t key, uint32_t value) {
    CHECK(key < UINT32_MAX , "Key can't be equal to UINT32_MAX");

    RMElement element = { .data = { .key = key, .value = value } };

    CHECK(map->end +1 < map->max, "RadixMap is full.");

    map->contents[map->end++] = element;

    RadixMap_sort(map);

    return 0;

error:
    return -1;
}

int RadixMap_delete(RadixMap *map, RMElement *el) {
    CHECK(map->end > 0, "There is nothing to delete.");
    CHECK(el != NULL, "Can't delete a NULL element.");

    el->data.key = UINT32_MAX;
    if (map->end > 1) {
        // don't bother resorting a map of 1 length
        RadixMap_sort(map);
    }

    map->end--;
    return 0;
error:
    return -1;
}
