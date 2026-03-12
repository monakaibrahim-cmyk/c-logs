#ifndef LOGS_H
#define LOGS_H

#define BLACK   "\033[30m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"

#define RESET   "\033[0m"

#define GET_INDENT_MACRO(_1, _2, _3, NAME, ...) NAME
#define indent(...) GET_INDENT_MACRO(__VA_ARGS__, _indent_file, _indent_stdout)(__VA_ARGS__)

typedef enum
{
    trace,
    debug,
    info,
    warn,
    error,
    fatal
} severity_level;

typedef struct Logger Logger;

struct Logger
{
    Logger* (*init)(const char* filename);
    Logger* (*set_level)(severity_level level);
    Logger* (*set_color)(int enable);
    void (*write)(severity_level level, const char* filename, int line, const char* function, const char* fmt, ...);
    void (*write_errors)(severity_level level, const char* filename, int line, const char* function, const char* fmt, ...);
};

extern Logger Logs;

#define LOG_TRACE(fmt, ...) \
    Logs.write(trace, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)

#define LOG_DEBUG(fmt, ...) \
    Logs.write(debug, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)

#define LOG_INFO(fmt, ...) \
    Logs.write(info, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)

#define LOG_WARN(fmt, ...) \
    Logs.write_errors(warn, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)

#define LOG_ERROR(fmt, ...) \
    Logs.write_errors(error, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)

#define LOG_FATAL(fmt, ...) \
    Logs.write_errors(fatal, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)

#endif // LOGS_H