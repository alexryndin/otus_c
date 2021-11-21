#ifndef _logger_h
#define _logger_h

enum { LOGGER_OK, LOGGER_ALREADY_INITIALIZED, LOGGER_E, LOGGER_MAX_SINKS };

int logger_init();

int logger_enable_stdout();
int logger_enable_file(const char *filepath);

enum { LOGGER_L_TRACE, LOGGER_L_DEBUG, LOGGER_L_INFO, LOGGER_L_WARN, LOGGER_L_ERROR, LOGGER_L_FATAL };
void logger_log(int level, const char *file, int line, const char *func, const char *fmt, ...);

#define LOGGER_TRACE(...) logger_log(LOGGER_L_TRACE, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOGGER_DEBUG(...) logger_log(LOGGER_L_DEBUG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOGGER_INFO(...)  logger_log(LOGGER_L_INFO,  __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOGGER_WARN(...)  logger_log(LOGGER_L_WARN,  __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOGGER_ERROR(...) logger_log(LOGGER_L_ERROR, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOGGER_FATAL(...) logger_log(LOGGER_L_FATAL, __FILE__, __LINE__, __func__, __VA_ARGS__)

int logger_deinit();

#define LOGGER_ERR LOGGER_ERR

#endif
