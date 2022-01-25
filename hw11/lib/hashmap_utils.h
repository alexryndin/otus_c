#ifndef hashmap_utils_h
#define hashmap_utils_h

#include <bstring/bstring/bstrlib.h>
#include <stdint.h>
#include <stdlib.h>

#define ByteOf(a, b) (((uint8_t *)a)[(b)])

#define JENKINS_HASH(ret, len, value, accessor) \
    for (ret = i = 0; i < (len); i++) {         \
        ret += accessor(value, i);              \
        ret += (ret << 10);                     \
        ret ^= (ret >> 6);                      \
    }                                           \
    ret += (ret << 3);                          \
    ret ^= (ret >> 11);                         \
    ret += (ret << 15)

typedef int (*Hashmap_compare)(void *a, void *b);
typedef uint32_t (*Hashmap_hash)(void *key);

#define GS_DEFAULT_MAX 4
#define GS_EXPAND_FACTOR 2

typedef struct GS {
    char *s;
    size_t len;
    size_t max;
} GS;

int gstr_cmp(GS *b1, GS *b2);
uint32_t gstr_hash(GS *key);

int str_cmp(bstring b1, bstring b2);
uint32_t str_hash(const bstring key);

int uint64_cmp(uint64_t *a, uint64_t *b);
int uint64_cmp_dec(uint64_t *a, uint64_t *b);

int uint32_cmp(uint32_t *a, uint32_t *b);
uint32_t uint32_hash(uint32_t *key);

GS *gs_create();

void gs_destroy(GS *gs);

int gsconchar(GS *gs, char ch);

#endif
