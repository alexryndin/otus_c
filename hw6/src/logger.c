#include <logger.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_SINKS 8

typedef struct Sink {
    FILE *f;
    char is_std;
} Sink;

static const char *level_strings[] = {"TRACE", "DEBUG", "INFO",
                                      "WARN",  "ERROR", "FATAL"};

static struct {
    char initialized;
    uint8_t cur_sinks;
    pthread_mutex_t m;
    Sink sinks[MAX_SINKS];
    void *btbuf[BT_LEN];
} Logger;

void **logger_get_btbuf() { return Logger.btbuf; }

int logger_enable_stdout() {
    int rc = 0;
    rc = pthread_mutex_lock(&Logger.m);
    if (rc != 0) {
        return LOGGER_E;
    }
    if (Logger.cur_sinks >= MAX_SINKS) {
        rc = pthread_mutex_unlock(&Logger.m);
        if (rc != 0) {
            return LOGGER_E;
        }
        return LOGGER_MAX_SINKS;
    }
    Sink s = {.f = stderr, .is_std = true};
    Logger.sinks[Logger.cur_sinks] = s;
    Logger.cur_sinks++;
    rc = pthread_mutex_unlock(&Logger.m);
    if (rc != 0) {
        return LOGGER_E;
    }
    return LOGGER_OK;
}

int logger_enable_file(const char *filepath) {
    int rc = 0;
    FILE *out_s = NULL;
    rc = pthread_mutex_lock(&Logger.m);
    if (rc != 0) {
        return LOGGER_E;
    }
    if (Logger.cur_sinks >= MAX_SINKS) {
        rc = pthread_mutex_unlock(&Logger.m);
        if (rc != 0) {
            return LOGGER_E;
        }
        return LOGGER_MAX_SINKS;
    }
    out_s = fopen(filepath, "a");
    if (out_s == NULL) {
        rc = pthread_mutex_unlock(&Logger.m);
        if (rc != 0) {
            return LOGGER_E;
        }
        return LOGGER_E;
    }
    Sink s = {.f = out_s, .is_std = false};
    Logger.sinks[Logger.cur_sinks] = s;
    Logger.cur_sinks++;
    rc = pthread_mutex_unlock(&Logger.m);
    if (rc != 0) {
        return LOGGER_E;
    }
    return LOGGER_OK;
}

int logger_init() {
    int rc = 0;

    if (Logger.initialized) {
        return LOGGER_ALREADY_INITIALIZED;
    }
    rc = pthread_mutex_init(&Logger.m, NULL);
    if (rc > 0) {
        return LOGGER_E;
    }
    Logger.cur_sinks = 0;
    Logger.initialized = true;
    return LOGGER_OK;
}

int logger_deinit() {
    int rc = 0;
    if (!Logger.initialized) {
        return LOGGER_OK;
    }
    rc = pthread_mutex_lock(&Logger.m);
    if (rc != 0) {
        return LOGGER_E;
    }
    for (int i = 0; i < Logger.cur_sinks; i++) {
        if (Logger.sinks[i].is_std) {
            Logger.sinks[i] = (Sink){0};
            continue;
        } else {
            rc = fclose(Logger.sinks[i].f);
            if (rc != 0) {
                return LOGGER_E;
            }
        }
    }
    Logger.cur_sinks = 0;
    rc = pthread_mutex_destroy(&Logger.m);
    if (rc > 0) {
        return LOGGER_E;
    }
    return LOGGER_OK;
}

int logger_lock() {
    int rc = 0;
    if (!Logger.initialized) {
        return LOGGER_E;
    }
    rc = pthread_mutex_lock(&Logger.m);
    if (rc != 0) {
        return LOGGER_E;
    }
    return LOGGER_OK;
}

#define timestamp_size 64
void logger_log_fatal(int level, const char *file, int line, const char *func,
                      int btn, const char *fmt, ...) {
    const int microsec_size = 10;
    int rc = 0;
    size_t written_ts = 0;
    char timestamp[timestamp_size] = {0};
    va_list args;
    struct timespec now;
    struct tm tm;
    if (!Logger.initialized) {
        return;
    }
    rc = clock_gettime(CLOCK_REALTIME, &now);
    if (rc == 0) {
        gmtime_r(&now.tv_sec, &tm);
        written_ts = strftime(timestamp, timestamp_size - microsec_size,
                              "%Y-%m-%dT%H:%M:%S.", &tm);
        written_ts +=
            snprintf(timestamp + written_ts, timestamp_size - written_ts,
                     "%09luZ", now.tv_nsec);
    }

    for (int i = 0; i < Logger.cur_sinks; i++) {
        va_start(args, fmt);
        fprintf(Logger.sinks[i].f, "%s [%s] at (%s:%d in %s): ", timestamp,
                level <= LOGGER_L_FATAL ? level_strings[level] : "", file, line,
                func);
        vfprintf(Logger.sinks[i].f, fmt, args);
        va_end(args);

        fprintf(Logger.sinks[i].f, "\n");
    }

    char **bts = backtrace_symbols(Logger.btbuf, btn);

    for (int s = 0; s < Logger.cur_sinks; s++) {
        fprintf(Logger.sinks[s].f, "Backtrace: \n");
        for (int bt = 0; bt < btn; bt++) {
            fprintf(Logger.sinks[s].f, "%s\n", bts[bt]);
        }
    }

    free(bts);
    rc = pthread_mutex_unlock(&Logger.m);
    if (rc != 0) {
        return;
    }
}

void logger_log(int level, const char *file, int line, const char *func,
                const char *fmt, ...) {
    const int microsec_size = 10;
    int rc = 0;
    size_t written_ts = 0;
    char timestamp[timestamp_size] = {0};
    va_list args;
    struct timespec now;
    struct tm tm;
    if (!Logger.initialized) {
        return;
    }
    rc = clock_gettime(CLOCK_REALTIME, &now);
    if (rc == 0) {
        gmtime_r(&now.tv_sec, &tm);
        written_ts = strftime(timestamp, timestamp_size - microsec_size,
                              "%Y-%m-%dT%H:%M:%S.", &tm);
        written_ts +=
            snprintf(timestamp + written_ts, timestamp_size - written_ts,
                     "%09luZ", now.tv_nsec);
    }

    rc = pthread_mutex_lock(&Logger.m);
    if (rc != 0) {
        return;
    }

    for (int i = 0; i < Logger.cur_sinks; i++) {
        va_start(args, fmt);
        fprintf(Logger.sinks[i].f, "%s [%s] at (%s:%d in %s): ", timestamp,
                level <= LOGGER_L_FATAL ? level_strings[level] : "", file, line,
                func);
        vfprintf(Logger.sinks[i].f, fmt, args);
        va_end(args);

        fprintf(Logger.sinks[i].f, "\n");
    }

    rc = pthread_mutex_unlock(&Logger.m);
    if (rc != 0) {
        return;
    }
}
