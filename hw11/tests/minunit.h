#undef NDEBUG
#ifndef _minunit_h
#define _minunit_h

#include <stdio.h>
#include <dbg.h>
#include <stdlib.h>

#define MU_SUITE_START() char *message = NULL

#define MU_ASSERT(test, message) if (!(test)) do { \
    LOG_ERR(message); \
    return message; \
} while (0)

#define MU_RUN_TEST(test) do { \
    LOG_DEBUG("\n-----%s", " " #test); \
    message = test(); \
    tests_run++; \
    if (message) return message; \
} while (0);

#define RUN_TESTS(name) int main(int argc, char *argv[]) { \
    (void)argc; \
    (void)argv; \
    LOG_DEBUG("----- RUNNING: %s", argv[0]); \
    printf("----\nRUNNING: %s\n", argv[0]); \
    char *result = name(); \
    if (result != 0) { \
        printf("FAILED: %s\n", result); \
    } else { \
        printf("ALL TESTS PASSED\n"); \
    } \
    printf("Tests run: %d\n", tests_run); \
    exit(result != 0); \
}

int tests_run;

#endif
