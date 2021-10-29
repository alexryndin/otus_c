#include "../src/radixmap.h"
#include "minunit.h"
#include <time.h>

static int make_random(RadixMap *map) {
    size_t i = 0;
    for (i = 0; i < map->max - 1; i++) {
        uint32_t key = (uint32_t)(rand() | (rand() << 16));
        CHECK(RadixMap_add(map, key, i) == 0, "Failed to add key %u.", key);
    }

    return i;
error:
    return 0;
}

static int check_order(RadixMap *map) {
    RMElement d1, d2;
    unsigned int i = 0;

    // only signal errors if any (should not be)
    for (i = 0; map->end > 0 && i < map->end - 1; i++) {
        d1 = map->contents[i];
        d2 = map->contents[i + 1];

        if (d1.data.key > d2.data.key) {
            LOG_DEBUG("FAIL:i=%u, key: %u, value: %u, equals max? %d", i,
                      d1.data.key, d1.data.value, d2.data.key == UINT32_MAX);
            return 0;
        }
    }

    return 1;
}

static int test_search(RadixMap *map) {
    unsigned i = 0;
    RMElement *d = NULL;
    RMElement *found = NULL;

    for (i = map->end / 2; i < map->end; i++) {
        d = &map->contents[i];
        found = RadixMap_find(map, d->data.key);
        CHECK(found != NULL, "Didn't find %u at %u.", d->data.key, i);
        CHECK(found->data.key == d->data.key,
              "Got the wrong result: %p:%u looking for %u at %u", found,
              found->data.key, d->data.key, i);
    }
    return 1;
error:
    return 0;
}

// test for big number of elements
static char *test_operations() {
    size_t N = 200;

    RadixMap *map = RadixMap_create(N);
    MU_ASSERT(map != NULL, "Failed to make the map.");
    MU_ASSERT(make_random(map), "Didn't make a random fake radix map.");

    RadixMap_sort(map);
    MU_ASSERT(check_order(map), "Failed to properly sort the RadixMap.");

    MU_ASSERT(test_search(map), "Failed the search test.");

    MU_ASSERT(check_order(map), "RadixMap didn't stay sorted after search.");

    while (map->end > 0) {
        RMElement *el =
            RadixMap_find(map, map->contents[map->end / 2].data.key);
        MU_ASSERT(el != NULL, "Should get a result.");

        size_t old_end = map->end;

        MU_ASSERT(RadixMap_delete(map, el)== 0, "Didn't delete it.");
        MU_ASSERT(old_end - 1 == map->end, "Wrong size after delete.");

        // test that the end is now the old value,
        // but uint32 max so it trails off
        MU_ASSERT(check_order(map), "RadixMap didn't stay sorted after delete.");
    }

    RadixMap_destroy(map);

    return NULL;
}

char *all_tests() {
    MU_SUITE_START();
    srand(time(NULL));

    MU_RUN_TEST(test_operations);

    return NULL;
}

RUN_TESTS(all_tests);
