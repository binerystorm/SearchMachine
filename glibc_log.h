#ifndef _GLIBC_LOG_H_
#define _GLIBC_LOG_H_
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

#endif // _GLIBC_LOG_H_
