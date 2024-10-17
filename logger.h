#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

// Logging levels
#define LOG_LEVEL_NONE  0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_INFO  3
#define LOG_LEVEL_DEBUG 4

// Set the current log level here
#define CURRENT_LOG_LEVEL LOG_LEVEL_DEBUG

// Log macros
#define LOG_ERROR(...) log_msg(LOG_LEVEL_ERROR, "ERROR", __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...)  log_msg(LOG_LEVEL_WARN,  "WARN",  __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...)  log_msg(LOG_LEVEL_INFO,  "INFO",  __FILE__, __LINE__, __VA_ARGS__)
#define LOG_DEBUG(...) log_msg(LOG_LEVEL_DEBUG, "DEBUG", __FILE__, __LINE__, __VA_ARGS__)

void log_msg(int level, const char* level_str, const char* file, int line, const char* format, ...);

#endif // LOGGER_H
