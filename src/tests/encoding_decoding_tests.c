// Created by RED on 18.12.2025.

#include "encoding_decoding_tests.h"

#include <stdlib.h>
#include <string.h>

#include "library.h"

#define TEST_WIDTH 512
#define TEST_HEIGHT 512

#define EQUALS_OR_ERROR(expr, msg, cleanup)\
    do {\
        if (!(expr)) {\
            cleanup;\
            return error_fatal(msg);\
        }\
    } while (0)


Error run_encoding_decoding_test() {
    srand(time(NULL));
    u8 image_buffer[TEST_WIDTH * TEST_HEIGHT * 3];

    for (int i = 0; i < TEST_WIDTH * TEST_HEIGHT * 3; ++i) {
        image_buffer[i] = rand() % 256;
    }
    PNGFile png = {0};
    png_from_data(image_buffer, TEST_WIDTH * TEST_HEIGHT * 3, TEST_WIDTH, TEST_HEIGHT, 3, 8, &png);
    MemoryFile memory_file = {0};
    UserIO io = {.read_func = memory_file_read, .write_func = memory_file_write, .user_file = &memory_file};
    PNGWriteConfig config;
    PNGWriteConfig_default(&config);
    RETURN_IF_FATAL_WITH_CLEANUP(png_write(&io, &config, &png), {
                                 MemoryFile_free(&memory_file);
                                 });

    memory_file.position = 0;
    PNGFile png_out;
    RETURN_IF_FATAL_WITH_CLEANUP(png_read(&io, &png_out), {
                                 MemoryFile_free(&memory_file);
                                 });

    EQUALS_OR_ERROR(png.bit_depth==png_out.bit_depth, "Bit depth mismatch after decoding", {
                    png_free(&png);
                    png_free(&png_out);
                    MemoryFile_free(&memory_file);
                    });

    EQUALS_OR_ERROR(png.color_type==png_out.color_type, "Color type mismatch after decoding", {
                    png_free(&png);
                    png_free(&png_out);
                    MemoryFile_free(&memory_file);
                    });

    EQUALS_OR_ERROR(png.width==png_out.width, "Width mismatch after decoding", {
                    png_free(&png);
                    png_free(&png_out);
                    MemoryFile_free(&memory_file);
                    });
    EQUALS_OR_ERROR(png.height==png_out.height, "Height mismatch after decoding", {
                    png_free(&png);
                    png_free(&png_out);
                    MemoryFile_free(&memory_file);
                    });

    if (memcmp(png.image_data, png_out.image_data, TEST_WIDTH * TEST_HEIGHT * 3) != 0) {
        png_free(&png);
        png_free(&png_out);
        if (memory_file.data != NULL) {
            free(memory_file.data);
            memory_file.data = NULL;
            memory_file.position = 0;
            memory_file.size = 0;
            memory_file.capacity = 0;
        }
        return error_fatal("Decoded image does not match the original");
    }

    MemoryFile_free(&memory_file);
    return NO_ERROR;
}

Error run_encoding_palette_generation_test() {
    u8 image_data[TEST_WIDTH * TEST_HEIGHT];
    for (int i = 0; i < TEST_HEIGHT * TEST_WIDTH; ++i) {
        image_data[i] = i % 256;
    }

    PNGWriteConfig config;
    PNGWriteConfig_default(&config);
    config.scan_palette = true;

    PNGFile png = {0};
    RETURN_IF_FATAL_WITH_CLEANUP(png_from_data(image_data, TEST_WIDTH*TEST_HEIGHT, TEST_WIDTH, TEST_HEIGHT, 1, 8, &png),
                                 {});

    MemoryFile memory_file = {0};
    UserIO io = {.read_func = memory_file_read, .write_func = memory_file_write, .user_file = &memory_file};
    RETURN_IF_FATAL_WITH_CLEANUP(png_write(&io, &config, &png), {
                                 MemoryFile_free(&memory_file);
                                 });

    memory_file.position = 0;
    PNGFile png_out = {0};
    RETURN_IF_FATAL_WITH_CLEANUP(png_read(&io, &png_out), {
                                 MemoryFile_free(&memory_file);
                                 });

    u8 check=0;

    for (int i = 0; i < 256; ++i) {
        check ^= png_out.palette[i * 4];
        check ^= i;
    }
    if (check != 0) {
        png_free(&png);
        png_free(&png_out);
        MemoryFile_free(&memory_file);
        return error_fatal("Palette data mismatch after encoding/decoding");
    }
    return NO_ERROR;
}
