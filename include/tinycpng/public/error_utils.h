// Created by RED on 09.12.2025.
#pragma once
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    // Must not be stored as can be backed by thread local global buffer
    const char *message;
    bool is_fatal;
} Error;

static const Error NO_ERROR = {NULL, false};

Error error_fatal(const char *message);

Error error_fatal_fmt(const char *format, ...);

Error error_warning(const char *message);

Error error_warning_fmt(const char *format, ...);

void error_exit_if_fatal(Error error);

#define RETURN_IF_FATAL_WITH_CLEANUP(err, cleanup_code_block) \
    do { \
        Error ___error = (err); \
        if (___error.is_fatal) { \
            {cleanup_code_block} \
            return ___error; \
        } \
        if (___error.message != NULL) {\
            fprintf(stderr, "Warning: %s\n", ___error.message);\
        }\
    } while (0)
