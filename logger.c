#include "logger.h"
#include <stdarg.h>
#include <time.h>
#include <string.h>

// Helper function to get the filename from a full path
const char* basename(const char* path) {
    const char* filename = strrchr(path, '/');
    if (filename == NULL) {
        filename = strrchr(path, '\\');
    }
    return filename ? filename + 1 : path;
}

void log_msg(int level, const char* level_str, const char* file, int line, const char* format, ...) {
    if (level <= CURRENT_LOG_LEVEL) {
        va_list args;
        time_t now = time(NULL);
        char time_str[20];

        strftime(time_str, sizeof(time_str), "%H:%M:%S", localtime(&now));
        fprintf(stderr, "%s [%s] %14s:%3d: ", time_str, level_str, basename(file), line);

        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);

        fprintf(stderr, "\n");
    }
}