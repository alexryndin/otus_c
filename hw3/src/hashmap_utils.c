#include "hashmap_utils.h"
#include <bstrlib/bstrlib.h>
#include <dbg.h>
#include <stdint.h>

typedef int (*Hashmap_compare)(void *a, void *b);
typedef uint32_t (*Hashmap_hash)(void *key);

int str_cmp(bstring b1, bstring b2) { return bstrcmp(b1, b2); }

int uint32_cmp(uint32_t *a, uint32_t *b) {
    if (*a < *b)
        return -1;
    if (*a > *b)
        return 1;
    return 0;
}

GS *gs_create() {
    char *s = calloc(GS_DEFAULT_MAX, sizeof(char));
    GS *ret = calloc(1, sizeof(GS));
    CHECK_MEM(s);
    CHECK_MEM(ret);

    ret->max = GS_DEFAULT_MAX;
    ret->len = 0;
    ret->s = s;

    return ret;

error:
    if (s != NULL)
        free(s);
    if (ret != NULL)
        free(ret);
    return NULL;
}

void gs_destroy(GS *gs) {
    CHECK(gs != NULL, "null good string.");
    free(gs->s);
    free(gs);
error:
    return;
}

int gsconchar(GS *gs, char ch) {
    void *rc = NULL;
    size_t new_max = 0;
    if (gs->len == (gs->max - 1)) {
        new_max = gs->max * GS_EXPAND_FACTOR;
        rc = realloc(gs->s, new_max);
        CHECK(rc != NULL, "good string reallocation failed.");
        memset((char *)rc + gs->len, 0, new_max - gs->len);
        gs->s = rc;
        gs->max = new_max;
    }

    gs->s[gs->len] = ch;

    gs->len++;

    return 0;
error:
    return 1;
}

int gstr_cmp(GS *b1, GS *b2) { return strcmp(b1->s, b2->s); }

uint32_t gstr_hash(GS *gs) {
    uint32_t ret, i;

    JENKINS_HASH(ret, gs->len, gs->s, ByteOf);
    return ret;
}

uint32_t str_hash(const bstring key) {
    if (blength(key) < 0) {
        LOG_ERR("Cannot get hash of a string of negative length.");
        return 0;
    }
    uint32_t ret, i;

    JENKINS_HASH(ret, (uint32_t)blength(key), key, bchar);
    return ret;
}

uint32_t uint32_hash(uint32_t *num) {
    uint32_t ret, i;

    JENKINS_HASH(ret, 4, num, ByteOf);
    return ret;
}
