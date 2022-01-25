#include "darray.h"
#include "dbg.h"

DArray *DArray_create(size_t initial_max) {
    DArray *ret = NULL;
    ret = calloc(1, sizeof(DArray));
    CHECK_MEM(ret);
    ret->end = 0;
    ret->max = initial_max;
    ret->expand_factor = DEFAULT_EXPAND_FACTOR;

    ret->contents = calloc(initial_max, sizeof(void *));
    CHECK_MEM(ret->contents);

    return ret;
error:
    if (ret != NULL) {
        if (ret->contents != NULL) {
            free(ret->contents);
        }
        free(ret);
    }
    return NULL;
}

void DArray_destroy(DArray *array) {
    CHECK(array != NULL, "Invalid array.");
    if (array->contents)
        free(array->contents);
    free(array);

error:
    return;
}

void DArray_set(DArray *array, size_t i, void *el) {
    CHECK(i < array->max, "array attempt to set past max.");
    if (i >= array->end)
        array->end = i + 1;
    ((void **)(array->contents))[i] = el;
error:
    return;
}

void *DArray_get(DArray *array, size_t i) {
    CHECK(i < array->max, "array attempt to get past max");
    return ((array->contents))[i];
error:
    return NULL;
}

void *DArray_remove(DArray *array, size_t i) {
    void *el;
    CHECK(i < array->max, "array attempt to get past max");
    el = ((array->contents))[i];

    ((array->contents))[i] = NULL;

    return el;
error:
    return NULL;
}

void DArray_clear(DArray *array) {
    CHECK(array != NULL, "Invalid array.");

    for (size_t i = 0; i < array->end; i++) {
        free(((void **)(array->contents))[i]);
        ((void **)(array->contents))[i] = NULL;
    }

error:
    return;
}

int DArray_expand(DArray *array) {
    CHECK(array != NULL, "Invalid array.");
    size_t new_size = DArray_max(array) * array->expand_factor + 1;
    CHECK(new_size > array->max && new_size > 0, "Bad new array size.");
    void *contents = realloc(array->contents, new_size * sizeof(void *));
    CHECK_MEM(contents);

    // treat contents as plain memory block
    memset((char *)contents + array->max * sizeof(void *), 0,
           (new_size - array->max) * sizeof(void *));

    array->max = new_size;
    array->contents = contents;

    return 0;
error:
    return -1;
}

int DArray_contract(DArray *array) {
    CHECK(array != NULL, "Invalid array.");
    size_t new_size = DArray_len(array);
    CHECK(new_size == array->end && new_size > 0, "Bad new array size.");
    void *contents = realloc(array->contents, new_size * sizeof(void *));
    CHECK_MEM(contents);

    array->max = new_size;
    array->contents = contents;

    return 0;
error:
    return -1;
}

int DArray_push(DArray *array, void *el) {
    CHECK(array != NULL, "Invalid array.");

    if (DArray_len(array) == DArray_max(array)) {
        CHECK(DArray_expand(array) == 0, "Couldn't expand array");
    }

    DArray_inc(array);
    DArray_last(array) = el;
    return 0;

error:
    return -1;
}

void *DArray_pop(DArray *array) {
    CHECK(array != NULL, "Invalid array.");
    CHECK(DArray_len(array) > 0, "Empty array.");
    void *ret = DArray_last(array);
    DArray_last(array) = NULL;
    DArray_dec(array);

    return ret;
error:
    return NULL;
}

void DArray_clear_destroy(DArray *array) {
    CHECK(array != NULL, "Invalid array.");

    for (size_t i = 0; i < array->end; i++) {
        free(((void **)(array->contents))[i]);
        ((void **)(array->contents))[i] = NULL;
    }
    free(array->contents);
    free(array);

error:
    return;
}
