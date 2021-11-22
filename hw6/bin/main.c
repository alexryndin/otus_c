#include <logger.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <dbg.h>

#define ITER_NUM 100000

const char *USAGE = "%s sink [sink...]\n";

static void *fuzzer(char *th) {
    size_t size = 256;
    char buf[256];
    int rnd;
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJK...";
    for (int i = 0; i < ITER_NUM; i++) {
        size = 256;
        if (size) {
            --size;
            rnd = rand();
            for (size_t n = 0; n < size; n++) {
                int key = rnd % (int)(sizeof charset - 1);
                buf[n] = charset[key];
                rnd++;
                if (rnd < 0) {
                    rnd = rand();
                }
            }
            buf[size] = '\0';
        }
        buf[0] = '%';
        buf[1] = 's';
        int level =
            rnd % (LOGGER_L_FATAL + 1 - LOGGER_L_TRACE) + LOGGER_L_TRACE;
        if (level < LOGGER_L_FATAL) {
            logger_log(level, __FILE__, __LINE__, __func__, buf, th);
        } else {
            LOGGER_FATAL(buf, th);
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    int rc = 0;
    pthread_t thread1, thread2;
    logger_init();
    if (argc < 2) {
        printf(USAGE, argv[0]);
        rc = 1;
        goto exit;
    }
    if (argc-1 > LOGGER_MAX_SINKS) {
        printf("Too many sinks, max = %d\n", LOGGER_MAX_SINKS);
        rc = 1;
        goto exit;
    }
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "stderr")) {
            logger_enable_stdout();
        } else {
            logger_enable_file(argv[i]);
        }
    }
    LOGGER_INFO("started");
    pthread_create(&thread1, NULL, (void * (*)(void *))fuzzer, "FIRST");
    pthread_create(&thread2, NULL, (void * (*)(void *))fuzzer, "SECOND");
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    LOGGER_INFO("ended");
    logger_deinit();

exit:
    return rc;
}
