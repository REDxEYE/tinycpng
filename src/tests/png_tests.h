// Created by RED on 11.12.2025.
#pragma once
#include "error_utils.h"

typedef enum {
    PNG_FILTER_TEST = 0,
    PNG_ENCODING_DECODING_TEST,
    PNG_PALETTE_GENERATION_TEST,

    PNG_TEST_COUNT
}PNGTests;

typedef Error (*PNGTestFunc)();


typedef struct {
    const char* name;
    PNGTests id;
    bool result;
    PNGTestFunc func;
}Test;


extern Test png_test_funcs[PNG_TEST_COUNT];