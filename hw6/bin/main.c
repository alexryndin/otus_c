#include <logger.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define ITER_NUM 10000

static void *fuzzer(char *th) {
    size_t size = 256;
    char buf[256];
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJK...";
    for (int i = 0; i < ITER_NUM; i++) {
        size = 256;
        if (size) {
            --size;
            for (size_t n = 0; n < size; n++) {
                int key = rand() % (int)(sizeof charset - 1);
                buf[n] = charset[key];
            }
            buf[size] = '\0';
        }
        buf[0] = '%';
        buf[1] = 's';
        int level =
            rand() % (LOGGER_L_FATAL + 1 - LOGGER_L_TRACE) + LOGGER_L_TRACE;
        if (level < LOGGER_L_FATAL) {
            logger_log(level, __FILE__, __LINE__, __func__, buf, th);
        } else {
            LOGGER_FATAL(buf, th);
        }
    }
}

int main(int argc, char *argv[]) {
    pthread_t thread1, thread2;
    logger_init();
    logger_enable_stdout();
    logger_enable_file("log.log");
    LOGGER_INFO("start");

    pthread_create(&thread1, NULL, fuzzer, "FIRST");
    pthread_create(&thread2, NULL, fuzzer, "SECOND");
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    LOGGER_INFO("end");
    logger_deinit();
    return 0;
error:
    return 1;
}
