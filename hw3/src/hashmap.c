#include "hashmap.h"
#include "hashmap_utils.h"
#include "../external/bstrlib/bstrlib.h"
#include "dbg.h"
#include "farray.h"


Hashmap *Hashmap_create(Hashmap_compare compare, Hashmap_hash hash) {
    size_t i = 0;
    FArray *new_bucket = NULL;
    Hashmap *ret = calloc(1, sizeof(Hashmap));
    CHECK_MEM(ret);

    ret->compare = compare != NULL ? compare : (Hashmap_compare)str_cmp;
    ret->hash = hash != NULL ? hash : (Hashmap_hash)str_hash;
    ret->number_of_buckets = DEFAULT_NUMBER_OF_BUCKETS;
    ret->buckets =
        FArray_create(sizeof(void *), sizeof(FArray), ret->number_of_buckets);
    CHECK(ret->buckets != NULL, "Couldn't initialize buckets.");

    for (i = 0; i < ret->number_of_buckets; i++) {
        new_bucket = FArray_create(sizeof(HashmapNode), 0, 5);
        CHECK(new_bucket != NULL, "Couldn't initialize bucket.");
        FArray_set_pointer(ret->buckets, i, new_bucket);
    }

    return ret;
error:
    if (ret != NULL) {
        if (ret->buckets != NULL) {
            for (i = 0; i < ret->number_of_buckets; i++) {
                new_bucket = FArray_get_pointer(ret->buckets, i);
                if (new_bucket != NULL) {
                    FArray_destroy(new_bucket);
                }
            }
            FArray_destroy(ret->buckets);
        }
        free(ret);
    }
    return NULL;
}

void Hashmap_destroy(Hashmap *map) {
    size_t i = 0;
    FArray *bucket = NULL;
    if (FArray_len(map->buckets) != map->number_of_buckets)
        LOG_ERR("number of buckets != len of buckets array.");
    for (i = 0; i < map->number_of_buckets; i++) {
        bucket = FArray_get_pointer(map->buckets, i);
        FArray_destroy(bucket);
    }
    FArray_destroy(map->buckets);
    free(map);
}

// HashmapNode *find_bucket(Hashmap *map, uint32_t key) {}

int Hashmap_set(Hashmap *map, void *key, void *data) {
    uint32_t hash = map->hash(key), i = 0;
    int rc = 0;
    FArray *bucket =
        FArray_get_pointer(map->buckets, hash % map->number_of_buckets);
    CHECK(bucket != NULL, "Couldn't find bucket.");
    LOG_DEBUG("bucket n %lu", hash % map->number_of_buckets);

    for (i = 0; i < FArray_len(bucket); i++) {
        HashmapNode *hmn = NULL;
        hmn = ((HashmapNode *)FArray_get(bucket, i));
        CHECK(hmn != NULL, "Couldn't get bucket node.");

        if (hash == hmn->hash && map->compare(key, hmn->key) == 0) {
            LOG_DEBUG("Found existing node, rewriting.");
            hmn->data = data;
            hmn->key = key;
            goto exit;
        }
    }

    rc = FArray_push(bucket,
                     &(HashmapNode){.key = key, .data = data, .hash = hash});
    LOG_DEBUG("pushed node %p", FArray_get(bucket, 0));
    LOG_DEBUG("Pushed %s",
              ((bstring)(((HashmapNode *)FArray_get(bucket, 0))->data))->data);

exit:
    return rc;
error:
    rc = -1;
    goto exit;
}

void *Hashmap_get(Hashmap *map, void *key) {
    void *ret = NULL;
    uint32_t hash = map->hash(key), i = 0, node_hash = 0;
    FArray *bucket =
        FArray_get_pointer(map->buckets, hash % map->number_of_buckets);
    LOG_DEBUG("bucket n %lu", hash % map->number_of_buckets);
    CHECK(bucket != NULL, "Couldn't find bucket.");
    LOG_DEBUG("bucket len %lu", FArray_len(bucket));

    for (i = 0; i < FArray_len(bucket); i++) {
        HashmapNode *hmn = NULL;
        hmn = ((HashmapNode *)FArray_get(bucket, i));
        CHECK(hmn != NULL, "Couldn't get bucket node.");

        node_hash = hmn->hash;
        LOG_DEBUG("node_hash %u", hmn->hash);
        LOG_DEBUG("key_hash %u", hash);
        LOG_DEBUG("key %s", ((bstring)(hmn->key))->data);
        LOG_DEBUG("compare %d", map->compare(key, hmn->key));
        if (hash == node_hash && map->compare(key, hmn->key) == 0) {
            ret = hmn->data;
            LOG_DEBUG("got %s", ((bstring)ret)->data);
            goto exit;
        }
    }

exit:
error:
    return ret;
}

int Hashmap_traverse(Hashmap *map, Hashmap_traverse_cb traverse_cb) {
    int rc = 0;
    size_t i = 0, j = 0;
    FArray *bucket = NULL;
    for (i = 0; i < map->number_of_buckets; i++) {
        bucket = FArray_get_pointer(map->buckets, i);
        CHECK(bucket != NULL, "Couldn't find bucket.");
        for (j = 0; j < FArray_len(bucket); j++) {
            HashmapNode *hmn = NULL;
            hmn = ((HashmapNode *)FArray_get(bucket, j));
            LOG_DEBUG("bucket %zu, node_hash %u, bucket len %zu", i, hmn->hash,
                      FArray_len(bucket));
            CHECK(hmn != NULL, "Couldn't get bucket node.");

            rc = traverse_cb(hmn);
            CHECK(rc == 0, "Callback failed.");
        }
    }

exit:
    return rc;
error:
    rc = -1;
    goto exit;
}

void *Hashmap_delete(Hashmap *map, void *key) {
    void *ret = NULL;
    uint32_t hash = map->hash(key), i = 0, node_hash = 0;
    FArray *bucket =
        FArray_get_pointer(map->buckets, hash % map->number_of_buckets);
    CHECK(bucket != NULL, "Couldn't find bucket.");

    for (i = 0; i < FArray_len(bucket); i++) {
        HashmapNode *hmn = NULL, *popped = NULL;
        hmn = ((HashmapNode *)FArray_get(bucket, i));
        CHECK(hmn != NULL, "Couldn't get bucket node.");

        node_hash = hmn->hash;
        if (hash == node_hash && map->compare(key, hmn->key) == 0) {
            ret = hmn->data;
            if (i < FArray_len(bucket) - 1) {
                popped = (HashmapNode *)FArray_pop(bucket);
                FArray_set(bucket, i, popped);
            } else {
                FArray_dec(bucket);
            }
            goto exit;
        }
    }

exit:
error:
    return ret;
}
