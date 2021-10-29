#include "../external/bstrlib/bstrlib.h"
#include "../src/hashmap.h"
#include "minunit.h"
#include <assert.h>

Hashmap *map = NULL;
static int traverse_called = 0;

struct tagbstring test1 = bsStatic("test data 1");
struct tagbstring test2 = bsStatic("test data 2");
struct tagbstring test3 = bsStatic("test data 3");
struct tagbstring expect1 = bsStatic("THE VALUE 1");
struct tagbstring expect2 = bsStatic("THE VALUE 2");
struct tagbstring expect3 = bsStatic("THE VALUE 3");

static int traverse_good_cb(HashmapNode *node) {
    LOG_DEBUG("KEY: %s", bdata((bstring)node->key));
    traverse_called++;
    return 0;
}

static int traverse_fail_cb(HashmapNode *node) {
    LOG_DEBUG("KEY: %s", bdata((bstring)node->key));
    traverse_called++;

    if (traverse_called == 2) {
        return 1;
    } else {
        return 0;
    }
    return -1;
}

char *test_create() {
    map = Hashmap_create(NULL, NULL);
    MU_ASSERT(map != NULL, "Failed to create map.");
    MU_ASSERT(map != NULL, "Failed to create map.");
    MU_ASSERT(map->hash != NULL, "Failed to create map.");
    MU_ASSERT(map->compare != NULL, "Failed to create map.");
    return NULL;
}

char *test_destroy() {
    Hashmap_destroy(map);

    return NULL;
}

char *test_get_set() {
    int rc = Hashmap_set(map, &test1, &expect1);
    MU_ASSERT(rc == 0, "Failed to set &test1");
    bstring result = Hashmap_get(map, &test1);
    MU_ASSERT(result == &expect1, "Wrong value for test1.");

    rc = Hashmap_set(map, &test2, &expect2);
    MU_ASSERT(rc == 0, "Failed to set &test2");
    result = Hashmap_get(map, &test2);
    MU_ASSERT(result == &expect2, "Wrong value for test2.");

    rc = Hashmap_set(map, &test3, &expect3);
    MU_ASSERT(rc == 0, "Failed to set &test3");
    result = Hashmap_get(map, &test3);
    MU_ASSERT(result == &expect3, "Wrong value for test3.");

    return NULL;
}

char *test_traverse() {
    int rc = Hashmap_traverse(map, traverse_good_cb);
    MU_ASSERT(rc == 0, "Failed to traverse.");
    MU_ASSERT(traverse_called == 3, "Wrong count traverse.");

    traverse_called = 0;
    rc = Hashmap_traverse(map, traverse_fail_cb);
    MU_ASSERT(rc == -1, "Failed to traverse.");
    MU_ASSERT(traverse_called == 2, "Wrong count traverse for fail.");

    return NULL;
}

char *test_delete() {
    bstring deleted = (bstring)Hashmap_delete(map, &test1);
    MU_ASSERT(deleted != NULL, "Got NULL on delete.");
    MU_ASSERT(deleted == &expect1, "Should get test1.");

    bstring result = Hashmap_get(map, &test1);
    MU_ASSERT(result == NULL, "Should delete.");

    deleted = (bstring)Hashmap_delete(map, &test2);
    MU_ASSERT(deleted != NULL, "Got NULL on delete.");
    MU_ASSERT(deleted == &expect2, "Should get test2.");

    result = Hashmap_get(map, &test2);
    MU_ASSERT(result == NULL, "Should delete.");

    deleted = (bstring)Hashmap_delete(map, &test3);
    MU_ASSERT(deleted != NULL, "Got NULL on delete.");
    MU_ASSERT(deleted == &expect3, "Should get test3.");

    result = Hashmap_get(map, &test3);
    MU_ASSERT(result == NULL, "Should delete.");

    return NULL;
}

char *all_tests() {
    MU_SUITE_START();

    MU_RUN_TEST(test_create);
    MU_RUN_TEST(test_get_set);
    MU_RUN_TEST(test_traverse);
    MU_RUN_TEST(test_delete);
    MU_RUN_TEST(test_destroy);

    return NULL;
}

RUN_TESTS(all_tests);
