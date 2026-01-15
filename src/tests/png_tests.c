// Created by RED on 11.12.2025.
#include "png_tests.h"

#include <stdio.h>

#include "encoding_decoding_tests.h"
#include "filter_tests.h"
#include "mytypes.h"

Test png_test_funcs[PNG_TEST_COUNT];

int main(s32 argc, char *argv[]) {
    png_test_funcs[PNG_FILTER_TEST] = (Test){
        "PNG Filter",
        PNG_FILTER_TEST,
        false,
        run_filter_tests
    };
    png_test_funcs[PNG_ENCODING_DECODING_TEST]= (Test){
        "PNG Encoding/Decoding",
        PNG_ENCODING_DECODING_TEST,
        false,
        run_encoding_decoding_test
    };
    png_test_funcs[PNG_PALETTE_GENERATION_TEST] = (Test){
        "PNG Palette Generation",
        PNG_PALETTE_GENERATION_TEST,
        false,
        run_encoding_palette_generation_test
    };

    bool all_tests_passed = true;
    for (int test_id = 0; test_id < PNG_TEST_COUNT; ++test_id) {
        if (png_test_funcs[test_id].func) {
            printf("Running %s test...\n", png_test_funcs[test_id].name);
            Error err = png_test_funcs[test_id].func();
            if (err.message != NULL) {
                printf("PNG Test %d failed: %s\n", test_id, err.message);
                all_tests_passed = false;
                png_test_funcs[test_id].result = false;
                continue;
            }
            png_test_funcs[test_id].result = true;
        }
    }

    // Print summary
    printf("\nPNG Test Summary:\n");
    for (int test_id = 0; test_id < PNG_TEST_COUNT; ++test_id) {
        if (png_test_funcs[test_id].func) {
            const char *status = png_test_funcs[test_id].result ? "PASSED" : "FAILED";
            printf("\tTest %s: %s\n", png_test_funcs[test_id].name, status);
        }
    }
    if (all_tests_passed) {
        printf("All PNG tests passed successfully.\n");
    } else {
        printf("Some PNG tests failed.\n");
    }

    return 0;
}
