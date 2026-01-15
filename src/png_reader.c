// Created by RED on 09.12.2025.

#include <stdio.h>
#include <time.h>

#include "library.h"
#include "file_utils.h"

u32 user_file_read(UserIO* user_io, void* out, const u32 out_size) {
    if (user_io->user_file==NULL) {
        return 0;
    }
    return (u32) fread(out, 1, out_size, (FILE *) user_io->user_file);
}

u32 user_file_write(UserIO* user_io, const void* in, const u32 in_size) {
    if (user_io->user_file==NULL) {
        return 0;
    }
    return (u32) fwrite(in, 1, in_size, (FILE *) user_io->user_file);
}

int main(int argc, char *argv[]) {
    // const char *filename = "D:/projects/cpp/tinycpng/samples/5,1_harder.png";
    // const char *filename = "D:/projects/cpp/tinycpng/samples/drive-sync16.png";
    // const char *filename = "D:/projects/cpp/tinycpng/samples/concrete_layers_02_diff_4k_simple.png";
    // const char *filename = "D:/projects/cpp/tinycpng/samples/concrete_layers_02_diff_4k.png";
    // const char *filename = "D:/projects/cpp/tinycpng/samples/Cobra_detail_displace.1003.png";
    // const char *filename = "D:/projects/cpp/tinycpng/samples/Cobra_detail_displace_4b.png";
    // const char *filename = "D:/projects/cpp/tinycpng/samples/Cobra_detail_displace_2b.png";
    const char *filename = "D:/projects/cpp/tinycpng/samples/dreadnought_body_01_emissive.ddsc_2EE0CCFD.png";
    // const char *filename = "D:/projects/cpp/tinycpng/samples/Cobra_detail_displace_1b.png";

    FILE *file = fopen(filename, "rb");

    if (!file) {
        printf("Failed to open file: %s\n", filename);
        return 1;
    }

    PNGFile png = {0};
    UserIO file_io = (UserIO){.read_func = user_file_read, .write_func = user_file_write, .user_file = file};
    size_t curr_time = clock();
    Error error = png_read(&file_io, &png);
    size_t end_time = clock();
    if (error.is_fatal) {
        png_free(&png);
    }
    fclose(file);
    error_exit_if_fatal(error);

    float time_taken = (f32) (end_time - curr_time) / (f32) CLOCKS_PER_SEC;
    printf("Loaded PNG: %ux%u, bit depth: %u, color type: %u in %.2f seconds\n",
           png.width, png.height, png.bit_depth, png.color_type,
           time_taken);
    printf("MPix/s: %.2f\n", (f32)png.width * (f32)png.height / (time_taken * 1000000.0f));

    if (png.palette != NULL) {
        error_exit_if_fatal(png_apply_palette(&png));
    }

    const char *w_filename = "D:/projects/cpp/tinycpng/out/test2.png";
    // const char *w_filename = "/mnt/d/projects/cpp/tinycpng/out/test2.png";

    FILE *w_file = fopen(w_filename, "wb");
    PNGWriteConfig config;
    PNGWriteConfig_default(&config);
    config.split_idat_chunks = true;
    config.split_idat_max_size = 65535*4;
    config.scan_palette = false;
    config.detect_gray = true;
    config.compression_level = PNG_DEFAULT_COMPRESSION;

    UserIO w_file_io = (UserIO){.read_func = user_file_read, .write_func = user_file_write, .user_file = w_file};
    curr_time = clock();
    png_write(&w_file_io, &config, &png);
    end_time = clock();
    time_taken = (f32) (end_time - curr_time) / (f32) CLOCKS_PER_SEC;
    printf("Written PNG: %ux%u, bit depth: %u, color type: %u in %.2f seconds\n",
           png.width, png.height, png.bit_depth, png.color_type,
           time_taken);
    printf("MPix/s: %.2f\n", (f32)png.width * (f32)png.height / (time_taken * 1000000.0f));

    png_free(&png);

    fclose(w_file);
    fclose(file);

    return 0;
}
