#ifndef RVEC_H
#define RVEC_H

#include <dbg.h>
#include <stdlib.h>

typedef enum {
    ERR_MEM = -1,
    ERR_NP = -2,
    ERR_OUT_OF_BOUNDS = -3,
} EC;

#define rvec_t(t) \
    struct {      \
        size_t n; \
        size_t m; \
        t *a;     \
    }

#define rv_init(v)  ((v).n = (v).m = 0, (v).a = NULL)
#define rv_at(v, i) ((v).a[(i)])
// #define rv_pop(v)     ((v).a[--(v).n])
#define rv_len(v) ((v).n)
#define rv_max(v) ((v).m)

#define REALLOC_FACTOR 1.5
#define rv_destroy(v)      \
    do {                   \
        if (v.a != NULL) { \
            free((v).a);   \
            (v).a = NULL;  \
            (v).m = 0;     \
            (v).n = 0;     \
        }                  \
    } while (0)

#define rv_resize(v, s, e)                             \
    do {                                               \
        void *swap;                                    \
        swap = realloc((v).a, sizeof(*(v).a) * (v).m); \
        if (swap == NULL) {                            \
            LOG_ERR("Vector reallocation failed");     \
            if (e != NULL) {                           \
                *e = ERR_MEM;                          \
            }                                          \
        }                                              \
        (v).a = swap;                                  \
        (v).m = (s);                                   \
    } while (0)

#define rv_push(vec, value, err)                                          \
    do {                                                                  \
        void *swap;                                                       \
        size_t new_length;                                                \
        if ((vec).n == (vec).m) {                                         \
            new_length = (vec).m <= 0 ? 2 : (vec).m * REALLOC_FACTOR + 1; \
            swap = realloc((vec).a, sizeof(*(vec).a) * new_length);       \
            if (swap == NULL) {                                           \
                LOG_ERR("Vector reallocation failed");                    \
                if (err != NULL) {                                        \
                    *(int *)err = ERR_MEM;                                \
                }                                                         \
            }                                                             \
            (vec).a = swap;                                               \
            (vec).m = new_length;                                         \
        }                                                                 \
        (vec).a[(vec).n++] = (value);                                     \
    } while (0)

#define rv_pop(v, e)                                                       \
    (((v).n <= (v).m) && ((v).n > 0)                                       \
         ? (v).a[--(v).n]                                                  \
         : (LOG_ERR("Out of bounds."),                                     \
            e != NULL                                                      \
                ? (*(int *)e = ERR_OUT_OF_BOUNDS, (__typeof__(*(v).a)){0}) \
                : (typeof(*(v).a)){0},                                     \
            (typeof(*(v).a)){0}))

#endif
