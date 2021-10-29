#ifndef hashmap_h
#define hashmap_h

#include "farray.h"
#include "hashmap_utils.h"
#include <stdint.h>

#define DEFAULT_NUMBER_OF_BUCKETS 100


typedef struct OpenHashmap {
    FArray *buckets;
    Hashmap_compare compare;
    Hashmap_hash hash;
    size_t number_of_buckets;
} Hashmap;

typedef struct HashmapNode {
    void *key;
    void *data;
    uint32_t hash;
} HashmapNode;

typedef int (*Hashmap_traverse_cb)(HashmapNode *node);

Hashmap *Hashmap_create(Hashmap_compare compare, Hashmap_hash hash);
void Hashmap_destroy(Hashmap *map);

int Hashmap_set(Hashmap *map, void *key, void *data);
void *Hashmap_get(Hashmap *map, void *key);

int Hashmap_traverse(Hashmap *map, Hashmap_traverse_cb traverse_cb);

void *Hashmap_delete(Hashmap *map, void *key);

#endif
