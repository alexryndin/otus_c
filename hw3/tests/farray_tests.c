#include <farray.h>
#include "minunit.h"
#include <stdint.h>

typedef struct ComplexStruct {
    uint64_t key;
    uint64_t data;
    uint32_t hash;
    char deleted;
} ComplexStruct;

char *test_simple_pointers() {
    FArray *array = NULL;
    int *val1 = NULL;
    int *val2 = NULL;
    int i = 0;
    int rc = 0;
    // create
    array = FArray_create(sizeof(void *), sizeof(int), 100);
    MU_ASSERT(array != NULL, "FArray_create failed.");
    MU_ASSERT(array->contents != NULL, "contents are wrong in array");
    MU_ASSERT(array->end == 0, "end isn't at the right spot");
    MU_ASSERT(array->element_size == sizeof(void *), "element size is wrong.");
    MU_ASSERT(array->max == 100, "wrong max length on initial size");

    // new
    val1 = FArray_new(array);
    MU_ASSERT(val1 != NULL, "failed to make a new element");

    val2 = FArray_new(array);
    MU_ASSERT(val2 != NULL, "failed to make a new element");

    // set
    FArray_set_pointer(array, 0, val1);
    FArray_set_pointer(array, 1, val2);

    // get
    MU_ASSERT(FArray_get_pointer(array, 0) == val1, "Wrong first value.");
    MU_ASSERT(FArray_get_pointer(array, 1) == val2, "Wrong second value.");

    // remove
    int *val_check = FArray_remove_pointer(array, 0);

    MU_ASSERT(val_check != NULL, "Should not get NULL.");
    MU_ASSERT(*val_check == *val1, "Should get the first value.");
    MU_ASSERT(FArray_get_pointer(array, 0) == NULL, "Should be gone.");
    FArray_free(val_check);

    val_check = FArray_remove_pointer(array, 1);
    MU_ASSERT(val_check != NULL, "Should not get NULL.");
    MU_ASSERT(*val_check == *val2, "Should get the second value.");
    MU_ASSERT(FArray_get_pointer(array, 1) == NULL, "Should be gone.");
    FArray_free(val_check);

    // expand
    int old_max = array->max;
    MU_ASSERT(FArray_expand(array) == 0, "Error on array expanding");
    MU_ASSERT(array->max == (size_t)(old_max * array->expand_factor + 1),
              "Wrong size after expand.");

    MU_ASSERT(FArray_contract(array) == 0, "Failed to contract");
    MU_ASSERT(array->max == array->end, "Array contraction failed.");
    MU_ASSERT(FArray_contract(array) == 0, "Failed to contract");
    MU_ASSERT(array->max == array->end, "Array contraction failed.");

    // push pop
    for (i = 0; i < 1000; i++) {
        int *val = FArray_new(array);
        *val = i * 333;
        rc = FArray_push_pointer(array, val);
        MU_ASSERT(rc == 0, "Error pushing pointer.");
    }

    LOG_DEBUG("%zu", array->max);
    MU_ASSERT(array->max == 1064, "Wrong max size.");

    for (i = 999; i >= 0; i--) {
        int *val = FArray_pop_pointer(array);
        MU_ASSERT(val != NULL, "Should't get a NULL");
        MU_ASSERT(*val == i * 333, "Wrong value.");
        FArray_free(val);
    }

    FArray_destroy(array);

error:
    return NULL;
}

char *test_ints() {
    FArray *array = NULL;
    int val = 0;
    int i = 0;
    int rc = 0;
    // create
    array = FArray_create(sizeof(int), 0, 100);
    MU_ASSERT(array != NULL, "FArray_create failed.");
    MU_ASSERT(array->contents != NULL, "contents are wrong in array");
    MU_ASSERT(array->end == 0, "end isn't at the right spot");
    MU_ASSERT(array->element_size == sizeof(int), "element size is wrong.");
    MU_ASSERT(array->storage_size == 0, "element size is wrong.");
    MU_ASSERT(array->max == 100, "wrong max length on initial size");

    for (i = 0; i < 1500; i++) {
        val = i * 333;
        rc = FArray_push(array, &val);
        MU_ASSERT(rc == 0, "Error pushing value.");
    }

    MU_ASSERT(array->max == 1732, "Wrong max size.");
    FArray_contract(array);
    MU_ASSERT(array->max == 1500, "Wrong max size.");

    for (i = 1499; i >= 0; i--) {
        int *val = FArray_pop(array);
        MU_ASSERT(val != NULL, "Should't get a NULL");
        MU_ASSERT(*val == i * 333, "Wrong value.");
    }

    FArray_destroy(array);
    return NULL;
}

char *test_ints_set() {
    FArray *array = NULL;
    int val = 0;
    int i = 0;
    int rc = 0;
    // create
    array = FArray_create(sizeof(int), 0, 100);
    MU_ASSERT(array != NULL, "FArray_create failed.");
    MU_ASSERT(array->contents != NULL, "contents are wrong in array");
    MU_ASSERT(array->end == 0, "end isn't at the right spot");
    MU_ASSERT(array->element_size == sizeof(int), "element size is wrong.");
    MU_ASSERT(array->storage_size == 0, "element size is wrong.");
    MU_ASSERT(array->max == 100, "wrong max length on initial size");

    for (i = 0; i < 100; i++) {
        val = i * 333;
        FArray_set(array, i, &val);
        MU_ASSERT(rc == 0, "Error pushing value.");
    }

    MU_ASSERT(array->max == 100, "Wrong max size.");
    MU_ASSERT(array->end == 100, "Wrong len size.");

    for (i = 99; i >= 0; i--) {
        int *val = FArray_get(array, i);
        MU_ASSERT(val != NULL, "Should't get a NULL");
        MU_ASSERT(*val == i * 333, "Wrong value.");
    }

    FArray_destroy(array);
    return NULL;
}

char *test_set_complex_structs() {
    FArray *array = NULL;
    int val = 0;
    uint64_t i = 0;
    int rc = 0;
    // create
    array = FArray_create(sizeof(ComplexStruct), 0, 100);
    MU_ASSERT(array != NULL, "FArray_create failed.");
    MU_ASSERT(array->contents != NULL, "contents are wrong in array");
    MU_ASSERT(array->end == 0, "end isn't at the right spot");
    MU_ASSERT(array->element_size == sizeof(ComplexStruct),
              "element size is wrong.");
    LOG_DEBUG("sizeof complexstruct is %lu", sizeof(ComplexStruct));
    MU_ASSERT(array->storage_size == 0, "element size is wrong.");
    MU_ASSERT(array->max == 100, "wrong max length on initial size");

    ComplexStruct ephemer = {0};
    for (i = 0; i < 100; i++) {
        ephemer.key = i;
        ephemer.data = (i * 333);
        FArray_set(array, i, &ephemer);
        MU_ASSERT(rc == 0, "Error pushing value.");
    }

    MU_ASSERT(array->max == 100, "Wrong max size.");
    MU_ASSERT(array->end == 100, "Wrong len size.");

    ComplexStruct *ret = NULL;

    for (int i = 99; i >= 0; i--) {
        LOG_DEBUG("i is %d", i);
        ret = FArray_get(array, i);
        MU_ASSERT(ret != NULL, "Should't get a NULL");
        MU_ASSERT(ret->key == i, "Wrong key value.");
        LOG_DEBUG("data %lu, key %lu, hash %d, deleted %c", ret->data, ret->key,
                  ret->hash, ret->deleted);
        MU_ASSERT(ret->data == i * 333, "Wrong data value.");
    }

    FArray_destroy(array);
    return NULL;
}

char *test_push_pop_complex_structs() {
    FArray *array = NULL;
    int val = 0;
    uint64_t i = 0;
    int rc = 0;
    // create
    array = FArray_create(sizeof(ComplexStruct), 0, 100);
    MU_ASSERT(array != NULL, "FArray_create failed.");
    MU_ASSERT(array->contents != NULL, "contents are wrong in array");
    MU_ASSERT(array->end == 0, "end isn't at the right spot");
    MU_ASSERT(array->element_size == sizeof(ComplexStruct),
              "element size is wrong.");
    LOG_DEBUG("sizeof complexstruct is %lu", sizeof(ComplexStruct));
    MU_ASSERT(array->storage_size == 0, "element size is wrong.");
    MU_ASSERT(array->max == 100, "wrong max length on initial size");

    ComplexStruct ephemer = {0};
    for (i = 0; i < 1000; i++) {
        ephemer.key = i;
        ephemer.data = (i * 333);
        rc = FArray_push(array, &ephemer);
        MU_ASSERT(rc == 0, "Error pushing value.");
    }

    MU_ASSERT(array->max == 1154, "Wrong max size.");
    MU_ASSERT(array->end == 1000, "Wrong len size.");

    ComplexStruct *ret = NULL;

    for (int i = 999; i >= 0; i--) {
        LOG_DEBUG("i is %d", i);
        ret = FArray_pop(array);
        MU_ASSERT(ret != NULL, "Should't get a NULL");
        MU_ASSERT(ret->key == i, "Wrong key value.");
        LOG_DEBUG("data %lu, key %lu, hash %d, deleted %c", ret->data, ret->key,
                  ret->hash, ret->deleted);
        MU_ASSERT(ret->data == i * 333, "Wrong data value.");
    }

    FArray_destroy(array);
    return NULL;
}

char *all_tests() {
    MU_SUITE_START();
    MU_RUN_TEST(test_simple_pointers);
    MU_RUN_TEST(test_ints);
    MU_RUN_TEST(test_ints_set);
    MU_RUN_TEST(test_set_complex_structs);
    MU_RUN_TEST(test_push_pop_complex_structs);

    return NULL;
}

RUN_TESTS(all_tests);
