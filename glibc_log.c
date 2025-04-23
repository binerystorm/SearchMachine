#include <stdio.h>
#include <stdarg.h>

#include "glibc_log.h"

// NOTE(gerick): this logging implementation has given in to using c stdlib functions for io and 
// exiting. This could cause conflicts when allocating memory with `mmap`. But I doubt it.

// TODO(gerick): implement ability to set log level.
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

static const char* level_to_str(Log_Level level)
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
