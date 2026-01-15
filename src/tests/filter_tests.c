// Created by RED on 11.12.2025.
#include "filter_tests.h"
#include "library.h"
#include "png_filters.h"

#include <stdio.h>
#include <stdlib.h>

#include "mytypes.h"

#define TEST_ROW_LENGTH 64

const char* PNGFilterTestNames[] = {
    "None",
    "Sub",
    "Up",
    "Average",
    "Paeth"
};

Error run_filter_tests() {
    srand(time(NULL));

    u8 test_row[TEST_ROW_LENGTH] = {0};
    u8 prev_test_row[TEST_ROW_LENGTH] = {0};
    u8 filtered_row[TEST_ROW_LENGTH] = {0};
    u8 unfiltered_row[TEST_ROW_LENGTH] = {0};

    for (int i = 0; i < TEST_ROW_LENGTH; ++i) {
        test_row[i] = rand() % 256;
        prev_test_row[i] = rand() % 256;
    }

    for (s32 i = 0; i<PNG_FILTER_TYPE_COUNT;i++) {
        printf("Testing filter %s\n", PNGFilterTestNames[i]);
        filters[i](test_row, prev_test_row, filtered_row, TEST_ROW_LENGTH, 1);
        unfilters[i](filtered_row, prev_test_row, unfiltered_row, TEST_ROW_LENGTH, 1);

        for (s32 j = 0; j<TEST_ROW_LENGTH;j++) {
            if (test_row[j] != unfiltered_row[j]) {
                return error_fatal_fmt("Filter test failed for filter type %d at index %d: expected %u, got %u",
                                       i, j, test_row[j], unfiltered_row[j]);
            }
        }
    }

    return NO_ERROR;
}
