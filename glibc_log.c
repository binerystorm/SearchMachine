#include <stdio.h>
#include <stdarg.h>

enum Log_Level {
    LOG_INFO,
    LOG_WARN,
    LOG_ERR,
};

#define Assert(pred, reason, ...) \
do{ \
    if(!(pred)){ \
        platform_assert(__FILE__, __func__, __LINE__, (#pred), reason __VA_OPT__(,) __VA_ARGS__); \
}}while(0)
#define ERROR(format, ...) do{ \
    platform_log(LOG_ERR, stderr, format __VA_OPT__(,) __VA_ARGS__); \
}while(0)
#define WARN(format, ...) do{ \
    platform_log(LOG_WARN, stderr, format __VA_OPT__(,) __VA_ARGS__); \
}while(0)
#define INFO(format, ...) do{ \
    platform_log(LOG_INFO, stdout, format __VA_OPT__(,) __VA_ARGS__); \
}while(0)

void platform_assert(
    const char *file_name, const char *fun_name,
    int line,
    const char *failure, const char* reason...)
{
    (void) fun_name;
    va_list args;
    va_start(args, reason);
    fprintf(stderr, "%s:%d: ", file_name, line);
    fprintf(stderr, "Assertion failed: \'%s\': ", failure);
    vfprintf(stderr, reason, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(1);
}

const char* level_to_str(Log_Level level)
{
    switch(level){
        case LOG_INFO: return "[\x1b[36mINFO\x1b[0m]";
        case LOG_WARN: return "[\x1b[33mWARN\x1b[0m]";
        case LOG_ERR: return "[\x1b[31mERR\x1b[0m]";
        default: printf("Unreachable"); exit(1);
    }
}

void platform_log(Log_Level level, FILE *file, 
                 const char *format, ...)
{
    va_list args;
    va_start(args, format);

    fprintf(file, "%s ", level_to_str(level));
    vfprintf(file, format, args);
    fprintf(file, "\n");
    va_end(args);
}
