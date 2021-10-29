#include "farray.h"
#include "dbg.h"

FArray *FArray_create(size_t element_size, size_t storage_size,
                      size_t initial_max) {
    FArray *ret = NULL;
    ret = calloc(1, sizeof(FArray));
    CHECK_MEM(ret);
    ret->end = 0;
    ret->max = initial_max;
    ret->element_size = element_size;
    ret->storage_size = storage_size;
    ret->expand_factor = DEFAULT_EXPAND_FACTOR;

    ret->contents = calloc(initial_max, element_size);
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

void FArray_destroy(FArray *array) {
    CHECK(array != NULL, "Invalid array.");
    if (array->contents)
        free(array->contents);
    free(array);

error:
    return;
}

void FArray_clear(FArray *array) {
    CHECK(array != NULL, "Invalid array.");
    CHECK(array->element_size == sizeof(void *),
          "Only ** arrays can be cleared.");

    for (int i = 0; i < array->end; i++) {
        free(((void **)(array->contents))[i]);
        ((void **)(array->contents))[i] = NULL;
    }

error:
    return;
}

int FArray_expand(FArray *array) {
    CHECK(array != NULL, "Invalid array.");
    size_t new_size = FArray_max(array) * array->expand_factor + 1;
    CHECK(new_size > array->max && new_size > 0, "Bad new array size.");
    void *contents =
        realloc(array->contents, new_size * sizeof(array->element_size));
    CHECK_MEM(contents);

    memset(contents + array->max * array->element_size, 0,
           (new_size - array->max) * array->element_size);

    array->max = new_size;
    array->contents = contents;

    return 0;
error:
    return -1;
}

int FArray_contract(FArray *array) {
    CHECK(array != NULL, "Invalid array.");
    size_t new_size = FArray_len(array);
    CHECK(new_size == array->end && new_size > 0, "Bad new array size.");
    void *contents =
        realloc(array->contents, new_size * sizeof(array->element_size));
    CHECK_MEM(contents);

    array->max = new_size;
    array->contents = contents;

    return 0;
error:
    return -1;
}

int FArray_push_pointer(FArray *array, void *el) {
    CHECK(array != NULL, "Invalid array.");
    CHECK(array->element_size == sizeof(void *),
          "Only ** arrays can be used to push a pointer in.");

    if (FArray_len(array) == FArray_max(array)) {
        CHECK(FArray_expand(array) == 0, "Couldn't expand array");
    }

    FArray_inc(array);
    FArray_last(array) = el;
    return 0;

error:
    return -1;
}

int FArray_push(FArray *array, void *el) {
    void *rc = 0, *dst = NULL;
    CHECK(array != NULL, "Invalid array.");

    if (FArray_len(array) == FArray_max(array)) {
        CHECK(FArray_expand(array) == 0, "Couldn't expand array");
    }

    dst = (void *)(array->contents + FArray_len(array) * array->element_size);
    LOG_DEBUG("%d", *(int *)el);

    rc = memcpy(dst, el, array->element_size);
    CHECK(rc == dst, "memcpy failed.");
    FArray_inc(array);
    return 0;

error:
    return -1;
}

void *FArray_pop_pointer(FArray *array) {
    CHECK(array != NULL, "Invalid array.");
    CHECK(FArray_len(array) > 0, "Empty array.");
    CHECK(array->element_size == sizeof(void *),
          "Only ** arrays can be used to pop pointer out.");
    void *ret = FArray_last(array);
    FArray_last(array) = NULL;
    FArray_dec(array);

    return ret;
error:
    return NULL;
}

void *FArray_pop(FArray *array) {
    CHECK(array != NULL, "Invalid array.");
    CHECK(FArray_len(array) > 0, "Empty array.");
    void *ret = (void *)(array->contents +
                         (FArray_len(array) - 1) * array->element_size);
    ;
    FArray_dec(array);

    return ret;
error:
    return NULL;
}

void FArray_clear_destroy(FArray *array) {
    CHECK(array != NULL, "Invalid array.");
    CHECK(array->element_size == sizeof(void *),
          "Only ** arrays can be cleared.");

    for (int i = 0; i < array->end; i++) {
        free(((void **)(array->contents))[i]);
        ((void **)(array->contents))[i] = NULL;
    }
    free(array->contents);
    free(array);

error:
    return;
}
