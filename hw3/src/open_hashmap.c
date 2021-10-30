#include "open_hashmap.h"
#include <bstrlib/bstrlib.h>
#include "dbg.h"
#include "farray.h"
#include "hashmap_utils.h"

OpenHashmap *_OpenHashmap_create(OpenHashmap_compare compare,
                                 OpenHashmap_hash hash,
                                 size_t number_of_buckets) {
    OpenHashmap *ret = calloc(1, sizeof(OpenHashmap));
    CHECK_MEM(ret);

    ret->compare = compare != NULL ? compare : (OpenHashmap_compare)str_cmp;
    ret->hash = hash != NULL ? hash : (OpenHashmap_hash)str_hash;
    ret->number_of_buckets = number_of_buckets;
    ret->realloc_factor = DEFAULT_REALLOC_FACTOR;
    ret->expand_factor = DEFAULT_EXPAND_FACTOR;
    ret->buckets =
        FArray_create(sizeof(OpenHashmapNode), 0, ret->number_of_buckets);
    CHECK(ret->buckets != NULL, "Couldn't initialize buckets.");
    // hack to set end to max
    OpenHashmapNode ephemer = {0};
    FArray_set(ret->buckets, ret->number_of_buckets - 1, &ephemer);
    CHECK(FArray_max(ret->buckets) == ret->number_of_buckets,
          "Couldn't initialize buckets.");
    CHECK(FArray_len(ret->buckets) == ret->number_of_buckets,
          "Couldn't initialize buckets.");
    CHECK(ret->number_of_buckets > 50, "Couldn't initialize map.");
    CHECK(ret->expand_factor > 1., "Couldn't initialize map.");
    CHECK(ret->realloc_factor < 1., "Couldn't initialize map.");
    return ret;
error:
    if (ret != NULL) {
        if (ret->buckets != NULL) {
            FArray_destroy(ret->buckets);
        }
        free(ret);
    }
    return NULL;
}

OpenHashmap *OpenHashmap_create(OpenHashmap_compare compare,
                                OpenHashmap_hash hash) {
    return _OpenHashmap_create(compare, hash, DEFAULT_NUMBER_OF_BUCKETS);
}

int OpenHashmap_destroy_with_kv(OpenHashmap *map) {
    int rc = 0;
    OpenHashmapNode *node = NULL;
    for (size_t i = 0; i < map->number_of_buckets; i++) {
        node = FArray_get(map->buckets, i);
        CHECK(node != NULL, "Couldn't find bucket.");
        if (node->key != NULL) {
            free(node->key);
            node->key = NULL;
        }
        if (node->data != NULL) {
            free(node->data);
            node->data = NULL;
        }
    }
    FArray_destroy(map->buckets);
    free(map);
error:
    return rc;
}

void OpenHashmap_destroy(OpenHashmap *map) {
    if (FArray_len(map->buckets) != map->number_of_buckets)
        LOG_ERR("number of buckets != len of buckets array.");
    FArray_destroy(map->buckets);
    free(map);
}

int OpenHashmap_realloc(OpenHashmap *map) {
    LOG_DEBUG("Reallocating map, number of buckets %zu, expand factor %f.",
              map->number_of_buckets, map->expand_factor);
    int rc = 0;
    OpenHashmap *new = NULL;
    CHECK(map != NULL, "Cannot reallocate null map.");
    new = _OpenHashmap_create(
        map->compare, map->hash,
        (size_t)(map->expand_factor * map->number_of_buckets));
    CHECK(new != NULL, "Couldn't create map to reallocate old.");

    OpenHashmapNode *node = NULL;
    for (size_t i = 0; i < map->number_of_buckets; i++) {
        node = FArray_get(map->buckets, i);
        CHECK(node != NULL, "Couldn't find bucket.");
        if (!IS_VACANT(node)) {
            rc = OpenHashmap_set(new, node->key, node->data);
            CHECK(rc == 0, "Couldn't add node to a new map.");
        }
    }

    // new map is created, now we use its guts to expand old map
    // :)
    FArray *swap = new->buckets;
    new->buckets = map->buckets;
    map->buckets = swap;

    size_t swap_n = new->number_of_buckets;
    new->number_of_buckets = map->number_of_buckets;
    map->number_of_buckets = swap_n;

error:
    if (new != NULL) {
        OpenHashmap_destroy(new);
    }
    return rc;
}
int OpenHashmap_set(OpenHashmap *map, void *key, void *data) {
    char ok = 0;
    uint32_t hash = map->hash(key);
    int rc = 0;
    size_t n_bucket = hash % map->number_of_buckets;
    size_t i = n_bucket;
    OpenHashmapNode *node = NULL;
    do {
        node = FArray_get(map->buckets, i);
        CHECK(node != NULL, "Couldn't find bucket.");
        if (IS_VACANT(node)) {
            node->data = data;
            node->hash = hash;
            node->key = key;
            node->deleted = 0;
            map->number_of_elements++;
            ok = 1;
        }
        i = (i + 1) % map->number_of_buckets;

    } while (i != n_bucket && !ok);

    if (map->number_of_elements >
        map->number_of_buckets * map->realloc_factor) {
        rc = OpenHashmap_realloc(map);
        CHECK(rc == 0, "Couldn't reallocate map.");
    }

    if (!ok) {
        LOG_ERR("Map seems to be full");
        goto error;
    }

exit:
    return rc;
error:
    rc = -1;
    goto exit;
}

void *OpenHashmap_get(OpenHashmap *map, void *key) {
    void *ret = NULL;
    uint32_t hash = map->hash(key);
    size_t n_bucket = hash % map->number_of_buckets;
    size_t i = n_bucket;
    OpenHashmapNode *node = NULL;
    do {
        node = FArray_get(map->buckets, i);
        CHECK(node != NULL, "Couldn't find bucket.");
        if (IS_VACANT(node)) {
            goto exit;
        }
        if (hash == node->hash && map->compare(key, node->key) == 0) {
            ret = node->data;
            goto exit;
        }
        i = (i + 1) % map->number_of_buckets;

    } while (i != n_bucket);

exit:
error:
    return ret;
}

int OpenHashmap_traverse(OpenHashmap *map,
                         OpenHashmap_traverse_cb traverse_cb) {
    int rc = 0;
    size_t i = 0;
    OpenHashmapNode *node = NULL;
    for (i = 0; i < map->number_of_buckets; i++) {
        node = FArray_get(map->buckets, i);
        CHECK(node != NULL, "Couldn't find bucket.");
        if (!IS_VACANT(node)) {
            rc = traverse_cb(node);
            CHECK(rc == 0, "Callback failed.");
        }
    }

exit:
    return rc;
error:
    rc = -1;
    goto exit;
}

void *OpenHashmap_delete(OpenHashmap *map, void *key) {
    void *ret = NULL;
    uint32_t hash = map->hash(key);
    size_t n_bucket = hash % map->number_of_buckets;
    size_t i = n_bucket;
    OpenHashmapNode *node = NULL;
    do {
        node = FArray_get(map->buckets, i);
        CHECK(node != NULL, "Couldn't find bucket.");
        LOG_DEBUG("bucket n %lu", hash % map->number_of_buckets);
        if (node->data == NULL || node->key == NULL || node->deleted) {
            goto exit;
        }
        if (hash == node->hash && map->compare(key, node->key) == 0) {
            ret = node->data;
            node->data = NULL;
            node->key = NULL;
            node->deleted = 1;
            LOG_DEBUG("got %s", ((bstring)ret)->data);
            goto exit;
        }
        i = (i + 1) % map->number_of_buckets;

    } while (i != n_bucket);

exit:
error:
    return ret;
}
