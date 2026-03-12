#ifndef LOGS_H
#define LOGS_H

/**
 * @file Logs.h
 * @brief Header file for the logging library.
 *
 * Provides a thread-safe logging facility with severity levels, colored output,
 * and optional file logging. Use the LOG_* macros for convenient logging.
 */

/**
 * @brief ANSI color codes for terminal text color.
 *
 * These macros define escape sequences for coloring log output in terminals
 * that support ANSI escape codes.
 */
/*@{*/
#define BLACK   "\033[30m"   ///< Black text color
#define RED     "\033[31m"   ///< Red text color (for errors)
#define GREEN   "\033[32m"   ///< Green text color (for success/info)
#define YELLOW  "\033[33m"   ///< Yellow text color (for warnings)
#define BLUE    "\033[34m"   ///< Blue text color
#define MAGENTA "\033[35m"   ///< Magenta text color (for fatal errors)
#define CYAN    "\033[36m"   ///< Cyan text color (for debug)
#define WHITE   "\033[37m"   ///< White text color (for trace)

#define RESET   "\033[0m"    ///< Reset color to default
/*@}*/

/**
 * @brief Helper macros for text indentation.
 *
 * These macros allow choosing between file and stdout output with
 * variable argument support. The GET_INDENT_MACRO uses C99 variadic
 * macro tricks to select the appropriate function.
 */
/*@{*/
#define GET_INDENT_MACRO(_1, _2, _3, NAME, ...) NAME
#define indent(...) GET_INDENT_MACRO(__VA_ARGS__, _indent_file, _indent_stdout)(__VA_ARGS__)
/*@}*/

/**
 * @brief Log severity levels.
 *
 * Defines the hierarchy of log message importance. Lower values indicate
 * less severe messages. The logger filters messages based on the configured
 * minimum level.
 */
typedef enum
{
    trace,  ///< Most verbose: detailed execution flow information
    debug,  ///< Debugging information for developers
    info,   ///< General informational messages
    warn,   ///< Warning messages indicating potential issues
    error,  ///< Error messages indicating failures
    fatal   ///< Critical errors that may cause program termination
} severity_level;

/**
 * @brief Forward declaration of the Logger structure.
 *
 * The Logger is an opaque structure that provides method chaining
 * for logger configuration and logging operations.
 */
typedef struct Logger Logger;

/**
 * @brief Logger structure with function pointers for logging operations.
 *
 * This struct provides a fluent API for configuring and using the logger.
 * Use method chaining to configure multiple options in sequence.
 *
 * Example:
 * @code
 * Logs.init("app.log")
 *    ->set_level(info)
 *    ->set_color(1);
 * @endcode
 */
struct Logger
{
    /**
     * @brief Initializes the logger with an optional log file.
     * @param[in] filename Path to log file, or NULL for stdout only.
     * @return Pointer to Logger for method chaining.
     */
    Logger* (*init)(const char* filename);

    /**
     * @brief Sets the minimum severity level to log.
     * @param[in] level The minimum severity level.
     * @return Pointer to Logger for method chaining.
     */
    Logger* (*set_level)(severity_level level);

    /**
     * @brief Enables or disables colored terminal output.
     * @param[in] enable Non-zero to enable, zero to disable.
     * @return Pointer to Logger for method chaining.
     */
    Logger* (*set_color)(int enable);

    /**
     * @brief Writes a standard log message.
     *
     * Only logs messages with severity >= the level set via set_level().
     * Messages below the configured level are silently ignored.
     *
     * @param[in] level Severity level of the message.
     * @param[in] filename Source file name.
     * @param[in] line Source line number.
     * @param[in] function Function name.
     * @param[in] fmt Format string.
     * @param[in] ... Arguments for the format string.
     */
    void (*write)(severity_level level, const char* filename, int line, const char* function, const char* fmt, ...);

    /**
     * @brief Writes an error message with stack trace.
     *
     * Only accepts warn, error, or fatal severity levels. Messages with
     * trace, debug, or info levels are silently ignored.
     *
     * @param[in] level Severity level (warn, error, or fatal).
     * @param[in] filename Source file name.
     * @param[in] line Source line number.
     * @param[in] function Function name.
     * @param[in] fmt Format string.
     * @param[in] ... Arguments for the format string.
     */
    void (*write_errors)(severity_level level, const char* filename, int line, const char* function, const char* fmt, ...);
};

/**
 * @brief Global Logger instance.
 *
 * Use this global instance to access logger functionality. The logger
 * must be initialized with Logs.init() before use.
 *
 * Example:
 * @code
 * Logs.init("app.log")->set_level(info)->set_color(1);
 * LOG_INFO("Application started");
 * @endcode
 */
extern Logger Logs;

/**
 * @brief Logs a message at TRACE severity level.
 *
 * Most verbose logging level. Used for detailed execution flow tracking.
 *
 * @param fmt Printf-style format string.
 * @param ... Arguments for the format string.
 */
#define LOG_TRACE(fmt, ...) \
    Logs.write(trace, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)

/**
 * @brief Logs a message at DEBUG severity level.
 *
 * Used for debugging information that helps developers understand
 * the program's execution.
 *
 * @param fmt Printf-style format string.
 * @param ... Arguments for the format string.
 */
#define LOG_DEBUG(fmt, ...) \
    Logs.write(debug, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)

/**
 * @brief Logs a message at INFO severity level.
 *
 * General informational messages about normal program operation.
 *
 * @param fmt Printf-style format string.
 * @param ... Arguments for the format string.
 */
#define LOG_INFO(fmt, ...) \
    Logs.write(info, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)

/**
 * @brief Logs a message at WARN severity level.
 *
 * Warning messages indicating potential issues that don't prevent
 * the program from continuing. Includes formatted error output with
 * stack trace.
 *
 * @param fmt Printf-style format string.
 * @param ... Arguments for the format string.
 */
#define LOG_WARN(fmt, ...) \
    Logs.write_errors(warn, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)

/**
 * @brief Logs a message at ERROR severity level.
 *
 * Error messages indicating failures in the program. Includes
 * formatted error output with stack trace.
 *
 * @param fmt Printf-style format string.
 * @param ... Arguments for the format string.
 */
#define LOG_ERROR(fmt, ...) \
    Logs.write_errors(error, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)

/**
 * @brief Logs a message at FATAL severity level.
 *
 * Critical errors that may cause program termination. Includes
 * formatted error output with stack trace.
 *
 * @param fmt Printf-style format string.
 * @param ... Arguments for the format string.
 */
#define LOG_FATAL(fmt, ...) \
    Logs.write_errors(fatal, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)

#endif // LOGS_H
