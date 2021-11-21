#include <logger.h>

int main(int argc, char *argv[]) {
    logger_init();
    logger_enable_stdout();
    logger_enable_file("log.log");
    LOGGER_INFO("test");
    return 0;
error:
    return 1;
}
