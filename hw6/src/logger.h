#ifndef _logger_h
#define _logger_h

#include <execinfo.h>

#define BT_LEN 20

enum { LOGGER_OK, LOGGER_ALREADY_INITIALIZED, LOGGER_E, LOGGER_MAX_SINKS };

int logger_init();

void **logger_get_btbuf();

int logger_lock();

int logger_enable_stdout();
int logger_enable_file(const char *filepath);

enum {
    LOGGER_L_TRACE,
    LOGGER_L_DEBUG,
    LOGGER_L_INFO,
    LOGGER_L_WARN,
    LOGGER_L_ERROR,
    LOGGER_L_FATAL
};
void logger_log(int level, const char *file, int line, const char *func,
                const char *fmt, ...);
void logger_log_fatal(int level, const char *file, int line, const char *func,
                      int btlen, const char *fmt, ...);

#define LOGGER_TRACE(...) \
    logger_log(LOGGER_L_TRACE, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOGGER_DEBUG(...) \
    logger_log(LOGGER_L_DEBUG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOGGER_INFO(...) \
    logger_log(LOGGER_L_INFO, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOGGER_WARN(...) \
    logger_log(LOGGER_L_WARN, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOGGER_ERROR(...) \
    logger_log(LOGGER_L_ERROR, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOGGER_FATAL(...)                                                   \
    do {                                                                    \
        int rc = logger_lock();                                             \
        if (rc != LOGGER_OK) {                                              \
            break;                                                          \
        }                                                                   \
        int btn = backtrace(logger_get_btbuf(), BT_LEN);                     \
        logger_log_fatal(LOGGER_L_FATAL, __FILE__, __LINE__, __func__, btn, \
                         __VA_ARGS__);                                      \
    } while (0)

int logger_deinit();

#define LOGGER_ERR LOGGER_ERR

#endif
