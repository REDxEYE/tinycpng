// Created by RED on 09.12.2025.

#include "error_utils.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

Error error_fatal(const char *message) {
    return (Error){.message = message, .is_fatal = true};
}

Error error_fatal_fmt(const char *format, ...) {
    thread_local static char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    return (Error){.message = buffer, .is_fatal = true};
}

Error error_warning(const char *message) {
    return (Error){.message = message, .is_fatal = false};
}

Error error_warning_fmt(const char *format, ...) {
    thread_local static char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    return (Error){.message = buffer, .is_fatal = false};
}

void error_exit_if_fatal(Error error) {
    if (error.is_fatal) {
        if (error.message) {
            fprintf(stderr, "Fatal Error: %s\n", error.message);
        } else {
            fprintf(stderr, "Fatal Error occurred.\n");
        }
        exit(EXIT_FAILURE);
    }
    if (error.message != NULL) {
        fprintf(stderr, "Warning: %s\n", error.message);
    }
}