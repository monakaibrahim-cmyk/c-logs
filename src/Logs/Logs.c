#include "Logs.h"

#include <execinfo.h>
#include <math.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

/**
 * @brief Internal logger state.
 *
 * Stores the runtime state of the logger including the log file handle,
 * current severity level, color enable flag, and mutex for thread safety.
 */
static struct
{
    FILE*           file;       ///< Log file handle (NULL if no file configured)
    severity_level  level;      ///< Minimum severity level to log
    int             enabled;    ///< Color output enabled flag (non-zero = enabled)
    pthread_mutex_t lock;       ///< Mutex for thread-safe logging
} State =
{
    NULL,
    debug,
    0,
    PTHREAD_MUTEX_INITIALIZER
};

/**
 * @brief String representations of severity levels.
 *
 * Used for displaying the severity level in log output.
 * Index corresponds to severity_level enum values.
 */
static const char* _lstr[] =
{
    "TRACE",
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"
};

/**
 * @brief ANSI color codes for each severity level.
 *
 * Maps each severity level to its corresponding ANSI color code
 * for terminal output when color is enabled.
 */
static const char* _cstr[] = 
{
    WHITE,   ///< trace - white
    CYAN,    ///< debug - cyan
    GREEN,   ///< info - green
    YELLOW,  ///< warn - yellow
    RED,     ///< error - red
    MAGENTA  ///< fatal - magenta
};

/**
 * @brief Initializes the logging system with an optional log file.
 *
 * This function opens a file for logging in append mode. If a filename is provided,
 * log messages will be written to both stdout and the specified file.
 * Thread-safe operation using mutex locking.
 *
 * @param[in] filename Path to the log file. If NULL, only stdout logging is used.
 * @return Pointer to the global Logger instance for method chaining.
 */
static Logger* init(const char* filename)
{
    pthread_mutex_lock(&State.lock);
    if (filename)
    {
        State.file = fopen(filename, "a");
    }

    pthread_mutex_unlock(&State.lock);
    return &Logs;
}

/**
 * @brief Sets the minimum severity level for logging.
 *
 * Only log messages with a severity level >= the specified level will be logged.
 * The default level is debug.
 *
 * @param[in] level The minimum severity level to log (trace, debug, info, warn, error, fatal).
 * @return Pointer to the global Logger instance for method chaining.
 */
static Logger* set_level(severity_level level)
{
    State.level = level;
    return &Logs;
}

/**
 * @brief Enables or disables colored output in the terminal.
 *
 * When enabled, log messages are printed with ANSI color codes for better
 * readability. Colors are mapped to severity levels.
 *
 * @param[in] enable Non-zero to enable colors, zero to disable.
 * @return Pointer to the global Logger instance for method chaining.
 */
static Logger* set_color(int enable)
{
    State.enabled = enable;
    return &Logs;
}

/**
 * @brief Internal worker function for text indentation and word wrapping.
 *
 * Writes text to the specified output stream with padding at the beginning of each line.
 * Implements word wrapping at the specified width boundary.
 *
 * @param[in,out] out File pointer to write the indented text to.
 * @param[in] padding Number of spaces to indent each line.
 * @param[in] text The text string to indent and wrap.
 * @param[in] width The column width at which to wrap long lines.
 */
static void _indent_worker(FILE* out, int padding, const char* text, int width)
{
    int current = 0;
    const char* p = text;

    fprintf(out, "%*s", padding, "");
    current += padding;

    while (*p)
    {
        if (*p == '\n')
        {
            fprintf(out, "\n%*s", padding, "");
            current = padding;
            p++;
            continue;
        }

        fprintf(out, "%c", *p);
        current++;
        p++;

        if (current >= width && *p == ' ')
        {
            fprintf(out, "\n%*s", padding, "");
            current = padding;
            p++;
        }
    }

    fprintf(out, "\n");
}

/**
 * @brief Indents and wraps text for file output.
 *
 * Wrapper around _indent_worker with fixed parameters for file output
 * (4-space indent, 80-character wrap width).
 *
 * @param[in,out] out File pointer to write the indented text to.
 * @param[in] padding Number of spaces to indent each line.
 * @param[in] text The text string to indent and wrap.
 */
static void _indent_file(FILE* out, int padding, const char* text)
{
    _indent_worker(out, 4, text, 80);
}

/**
 * @brief Indents and wraps text for stdout output.
 *
 * Wrapper around _indent_worker with fixed parameters for stdout
 * (4-space indent, 80-character wrap width).
 *
 * @param[in] padding Number of spaces to indent each line.
 * @param[in] text The text string to indent and wrap.
 */
static void _indent_stdout(int padding, const char* text)
{
    _indent_worker(stdout, 4, text, 80);
}

/**
 * @brief Prints a stack trace with indentation.
 *
 * Captures the current call stack using backtrace() and prints it
 * with the specified indentation. Used for error and fatal log messages
 * to help with debugging.
 *
 * @param[in,out] out File pointer to write the stack trace to.
 * @param[in] padding Number of spaces to indent each line of the stack trace.
 */
static void _indent_stack_trace(FILE* out, int padding)
{
    void* array[32];
    int size = backtrace(array, 32);
    char** symbols = backtrace_symbols(array, size);

    if (symbols != NULL)
    {
        fprintf(out, "Stack Trace:\n");
        
        for (int i = 0; i < size; i++)
        {
            fprintf(out, "%*s%s\n", padding, "", symbols[i]);
        }
        
        free(symbols);
    }
}

/**
 * @brief Writes a standard log message.
 *
 * Formats and outputs a log message with timestamp, severity level, file location,
 * function name, and the formatted message. Thread-safe using mutex locking.
 * Logs to both stdout and file (if configured).
 *
 * Only logs messages with severity >= the level set via set_level().
 * Messages below the configured level are silently ignored.
 *
 * @param[in] level Severity level of the message (trace, debug, info, warn, error, fatal).
 * @param[in] filename Source file where the log call originated (usually __FILE__).
 * @param[in] line Line number in the source file (usually __LINE__).
 * @param[in] function Name of the function containing the log call (usually __FUNCTION__).
 * @param[in] fmt Printf-style format string for the message.
 * @param[in] ... Additional arguments for the format string.
 */
static void writec(severity_level level, const char* filename, int line, const char* function, const char* fmt, ...)
{
    pthread_mutex_lock(&State.lock);

    if (level < State.level)
    {
        pthread_mutex_unlock(&State.lock);
        return;
    }

    char buffer[30];
    struct timeval tv;
    gettimeofday(&tv, NULL);

    int milliseconds = lrint(tv.tv_usec / 1000.0);

    if (milliseconds >= 1000)
    {
        milliseconds -= 1000;
        tv.tv_sec++;
    }

    struct tm* t = localtime(&tv.tv_sec);

    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", t);

    char msg[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    if (State.enabled)
    {
        printf(
            "%s.%03d [%s%s%s] [%s%s%s:%s%d%s] [%s%s%s] %s\n",
            buffer,
            milliseconds,
            _cstr[level],
            _lstr[level],
            RESET,
            YELLOW,
            filename,
            RESET,
            YELLOW,
            line,
            RESET,
            YELLOW,
            function,
            RESET,
            msg
        );
    }
    else
    {
        printf(
            "%s.%03d [%s] [%s:%d] [%s] %s\n", 
            buffer,
            milliseconds,
            _lstr[level],
            filename,
            line,
            function,
            msg
        );
    }

    fflush(stdout);

    if (State.file != NULL)
    {
        fprintf(
            State.file,
            "%s.%03d [%s] [%s:%d] [%s] %s\n", 
            buffer,
            milliseconds,
            _lstr[level],
            filename,
            line,
            function,
            msg
        );
        fflush(State.file);
    }

    pthread_mutex_unlock(&State.lock);
}

/**
 * @brief Writes an error log message with detailed formatting and stack trace.
 *
 * Outputs a formatted error message with a boxed header/footer, timestamp,
 * thread ID, file location, and a stack trace. Used for warn, error, and fatal
 * severity levels to provide more debugging information. Thread-safe using
 * mutex locking.
 *
 * Only accepts warn, error, or fatal severity levels. Messages with
 * trace, debug, or info levels are silently ignored.
 *
 * @param[in] level Severity level of the message (warn, error, or fatal).
 * @param[in] filename Source file where the log call originated (usually __FILE__).
 * @param[in] line Line number in the source file (usually __LINE__).
 * @param[in] function Name of the function containing the log call (usually __FUNCTION__).
 * @param[in] fmt Printf-style format string for the message.
 * @param[in] ... Additional arguments for the format string.
 */
static void write_errors(severity_level level, const char* filename, int line, const char* function, const char* fmt, ...)
{
    pthread_mutex_lock(&State.lock);

    if (level < warn)
    {
        pthread_mutex_unlock(&State.lock);
        return;
    }

    char buffer[30];
    struct timeval tv;
    gettimeofday(&tv, NULL);

    int milliseconds = lrint(tv.tv_usec / 1000.0);

    if (milliseconds >= 1000)
    {
        milliseconds -= 1000;
        tv.tv_sec++;
    }

    struct tm* t = localtime(&tv.tv_sec);

    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", t);

    char msg[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    const char* header = "\n================================================\n";
    const char* footer = "================================================\n\n";
    const char* horizontal = "------------------------------------------------";

    {
        printf("%s", header);

        if (State.enabled)
        {
            printf("Timestamp: %-10s.%03d\n", buffer, milliseconds);
            printf("Severity:  %s%-10s%s\n", _cstr[level], _lstr[level], RESET);
            printf("Thread:    %s%-10lu%s\n", GREEN, (unsigned long)pthread_self(), RESET);
            printf("Location:  %s%-10s%s:%s%d%s (%s)\n", YELLOW, filename, RESET, YELLOW, line, RESET, function);
        }
        else
        {
            printf("Timestamp: %-10s.%03d\n", buffer, milliseconds);
            printf("Severity:  %-10s\n", _lstr[level]);
            printf("Thread:    %-10lu\n", (unsigned long)pthread_self());
            printf("Location:  %-10s:%d (%s)\n", filename, line, function);
        }

        printf("Message:\n");
        indent(4, msg);
        printf("%s\n", horizontal);
        _indent_stack_trace(stdout, 4);
        printf("%s", footer);
        fflush(stdout);
    }

    if (State.file != NULL)
    {
        fprintf(State.file, "%s", header);
        fprintf(State.file, "Timestamp: %-10s.%03d\n", buffer, milliseconds);
        fprintf(State.file, "Severity:  %-10s\n", _lstr[level]);
        fprintf(State.file, "Thread:    %-10lu\n", (unsigned long)pthread_self());
        fprintf(State.file, "Location:  %-10s:%d (%s)\n", filename, line, function);
        fprintf(State.file, "Message:\n");
        indent(State.file, 4, msg);

        fprintf(State.file, "%s\n", horizontal);
        fflush(State.file);

        _indent_stack_trace(State.file, 4);
        fprintf(State.file, "%s", footer);
        fflush(State.file);
    }

    pthread_mutex_unlock(&State.lock);
}

/**
 * @brief Global Logger instance.
 *
 * This is the main entry point for using the logging library.
 * Initialize with Logs.init() before use, then use the LOG_* macros
 * or directly call Logs.write()/Logs.write_errors() for logging.
 */
Logger Logs =
{
    .init = init,
    .set_level = set_level,
    .set_color = set_color,
    .write = writec,
    .write_errors = write_errors
};
