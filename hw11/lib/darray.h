#ifndef _darray_h
#define _darray_h
#include "dbg.h"
#include <stdlib.h>

typedef struct DArray {
    size_t end;
    size_t max;
    double expand_factor;
    void **contents;
} DArray;

DArray *DArray_create(size_t initial_max);

void DArray_destroy(DArray *array);

void DArray_clear(DArray *array);

int DArray_expand(DArray *array);

int DArray_contract(DArray *array);

int DArray_push(DArray *array, void *el);

void *DArray_pop(DArray *array);

void DArray_clear_destroy(DArray *array);

#define DArray_last(A) ((A)->contents)[(A)->end - 1]
#define DArray_index(A, i) (((A)->contents)[i])
#define DArray_first(A) ((A)->contents[0])
#define DArray_len(A) ((A)->end)
#define DArray_count(A) DArray_end(A)
#define DArray_max(A) ((A)->max)
#define DArray_inc(A) ((A)->end++)
#define DArray_dec(A) ((A)->end--)

#define DEFAULT_EXPAND_FACTOR 1.5;

void DArray_set(DArray *array, size_t i, void *el);

void *DArray_get(DArray *array, size_t i);

void *DArray_remove(DArray *array, size_t i);

void *DArray_new(DArray *array);

#define DArray_free(E)                                                         \
    do {                                                                       \
        free((E));                                                             \
    } while (0)

#endif
