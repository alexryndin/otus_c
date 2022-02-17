#ifndef __dbg_h__
#define __dbg_h__

#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifdef NDEBUG
#define LOG_DEBUG(M, ...)
#else
#define LOG_DEBUG(M, ...)                                               \
    fprintf(stderr, "[DEBUG] %s:%d in %s: " M "\n", __FILE__, __LINE__, \
            __func__, ##__VA_ARGS__)
#endif

#define CLEAN_ERRNO() (errno == 0 ? "None" : strerror(errno))

#define LOG_ERR(M, ...)                                                   \
    fprintf(stderr, "[ERROR] (%s:%d in %s: errno: %s) " M "\n", __FILE__, \
            __LINE__, __func__, CLEAN_ERRNO(), ##__VA_ARGS__)

#define LOG_WARN(M, ...)                                                 \
    fprintf(stderr, "[WARN] (%s:%d in %s: errno: %s) " M "\n", __FILE__, \
            __LINE__, __func__, CLEAN_ERRNO(), ##__VA_ARGS__)

#define LOG_INFO(M, ...)                                                \
    fprintf(stderr, "[INFO] (%s:%d in %s) " M "\n", __FILE__, __LINE__, \
            __func__, ##__VA_ARGS__)

#define CHECK(A, M, ...)               \
    do {                               \
        if (!(A)) {                    \
            LOG_ERR(M, ##__VA_ARGS__); \
            errno = 0;                 \
            goto error;                \
        }                              \
    } while (0)

#define CHECKRC(A, RC, M, ...)         \
    do {                               \
        if (!(A)) {                    \
            rc = RC;                   \
            LOG_ERR(M, ##__VA_ARGS__); \
            errno = 0;                 \
            goto error;                \
        }                              \
    } while (0)

#define SENTINEL(M, ...)           \
    do {                           \
        LOG_ERR(M, ##__VA_ARGS__); \
        errno = 0;                 \
        goto error;                \
    } while (0)

#define CHECK_MEM(A) CHECK((A), "Out of memory.")

#define CHECK_DEBUG(A, M, ...)           \
    do {                                 \
        if (!(A)) {                      \
            LOG_DEBUG(M, ##__VA_ARGS__); \
            errno = 0;                   \
            goto error;                  \
        }                                \
    } while (0)

#endif
