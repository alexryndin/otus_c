#include <darray.h>
#include "minunit.h"
#include <stdint.h>

typedef struct ComplexStruct {
    uint64_t key;
    uint64_t data;
    uint32_t hash;
    char deleted;
} ComplexStruct;

char *test_simple_pointers() {
    DArray *array = NULL;
    int val1 = 43;
    int val2  = 54;
    int *val1_ptr = &val1;
    int *val2_ptr = &val2;
    int i = 0;
    int rc = 0;
    // create
    array = DArray_create(100);
    MU_ASSERT(array != NULL, "DArray_create failed.");
    MU_ASSERT(array->contents != NULL, "contents are wrong in array");
    MU_ASSERT(array->end == 0, "end isn't at the right spot");
    MU_ASSERT(array->max == 100, "wrong max length on initial size");

    // set
    DArray_set(array, 0, val1_ptr);
    DArray_set(array, 1, val2_ptr);

    // get
    MU_ASSERT(DArray_get(array, 0) == val1_ptr, "Wrong first value.");
    MU_ASSERT(DArray_get(array, 1) == val2_ptr, "Wrong second value.");
    MU_ASSERT(*(int *)DArray_get(array, 0) == val1, "Wrong first value.");
    MU_ASSERT(*(int *)DArray_get(array, 1) == val2, "Wrong second value.");

    // remove
    int *val_check = DArray_remove(array, 0);
    MU_ASSERT(val_check != NULL, "Should not get NULL.");
    MU_ASSERT(*val_check == val1, "Should get the first value.");
    MU_ASSERT(DArray_get(array, 0) == NULL, "Should be gone.");

    val_check = DArray_remove(array, 1);
    MU_ASSERT(val_check != NULL, "Should not get NULL.");
    MU_ASSERT(*val_check == val2, "Should get the second value.");
    MU_ASSERT(DArray_get(array, 1) == NULL, "Should be gone.");

    // expand
    int old_max = array->max;
    MU_ASSERT(DArray_expand(array) == 0, "Error on array expanding");
    MU_ASSERT(array->max == (size_t)(old_max * array->expand_factor + 1),
              "Wrong size after expand.");

    MU_ASSERT(DArray_contract(array) == 0, "Failed to contract");
    MU_ASSERT(array->max == array->end, "Array contraction failed.");
    MU_ASSERT(DArray_contract(array) == 0, "Failed to contract");
    MU_ASSERT(array->max == array->end, "Array contraction failed.");

    // push pop
    for (i = 0; i < 1000; i++) {
        int *val = malloc(sizeof(int));
        CHECK_MEM(val);
        *val = i * 333;
        rc = DArray_push(array, val);
        MU_ASSERT(rc == 0, "Error pushing pointer.");
    }

    LOG_DEBUG("%zu", array->max);
    MU_ASSERT(array->max == 1064, "Wrong max size.");

    for (i = 999; i >= 0; i--) {
        int *val = DArray_pop(array);
        MU_ASSERT(val != NULL, "Should't get a NULL");
        MU_ASSERT(*val == i * 333, "Wrong value.");
        DArray_free(val);
    }

    DArray_destroy(array);

error:
    return NULL;
}

char *test_pointers() {
    DArray *array = NULL;
    array = DArray_create(1);
    int test1 = 6;
    int test2 = 42;
    int test3 = 100000000;

    int *test1_ptr = &test1;
    int *test2_ptr = &test2;
    int *test3_ptr = &test3;

    LOG_DEBUG("test1 address %p", &test1);

    DArray_push(array, &test1);
    DArray_push(array, test2_ptr);
    DArray_push(array, &test3);


    MU_ASSERT((int *)DArray_get(array, 0) == &test1, "Wrong address in array");
    MU_ASSERT((int *)DArray_get(array, 0) == &test1, "Wrong address in array");
    MU_ASSERT((int *)DArray_get(array, 1) == &test2, "Wrong address in array");
    MU_ASSERT((int *)DArray_get(array, 2) == &test3, "Wrong address in array");
    MU_ASSERT((int *)DArray_get(array, 2) == &test3, "Wrong address in array");

    MU_ASSERT(*(int *)DArray_get(array, 2) == test3, "Wrong value in array");
    MU_ASSERT(*(int *)DArray_get(array, 0) == test1, "Wrong value in array");
    MU_ASSERT(*(int *)DArray_get(array, 1) == test2, "Wrong value in array");

    return NULL;
}

char *all_tests() {
    MU_SUITE_START();
    MU_RUN_TEST(test_simple_pointers);
    MU_RUN_TEST(test_pointers);

    return NULL;
}

RUN_TESTS(all_tests);
