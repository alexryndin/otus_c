#ifndef open_hashmap_h
#define open_hashmap_h

#include "farray.h"
#include <stdint.h>

#define DEFAULT_NUMBER_OF_BUCKETS 100
#define DEFAULT_REALLOC_FACTOR 0.9

typedef int (*OpenHashmap_compare)(void *a, void *b);
typedef uint32_t (*OpenHashmap_hash)(void *key);

#define IS_VACANT(n) ((n)->data == NULL || (n)->key == NULL || (n)->deleted)

typedef struct OpenHashmap {
    FArray *buckets;
    OpenHashmap_compare compare;
    OpenHashmap_hash hash;
    size_t number_of_buckets;
    size_t number_of_elements;
    double realloc_factor;
    double expand_factor;
} OpenHashmap;

typedef struct OpenHashmapNode {
    void *key;
    void *data;
    uint32_t hash;
    char deleted;
} OpenHashmapNode;

typedef int (*OpenHashmap_traverse_cb)(OpenHashmapNode *node);

OpenHashmap *OpenHashmap_create(OpenHashmap_compare compare,
                                OpenHashmap_hash hash);
void OpenHashmap_destroy(OpenHashmap *map);

int OpenHashmap_destroy_with_kv(OpenHashmap *map);

int OpenHashmap_set(OpenHashmap *map, void *key, void *data);
void *OpenHashmap_get(OpenHashmap *map, void *key);

int OpenHashmap_traverse(OpenHashmap *map, OpenHashmap_traverse_cb traverse_cb);

void *OpenHashmap_delete(OpenHashmap *map, void *key);

#endif
