// Created by RED on 15.12.2025.
#include "png_write.h"

#include <math.h>
#include <string.h>

#include "file_utils.h"
#include "png_filters.h"
#include "png_helpers.h"
#include "png_palette.h"
#include "zlib-ng.h"
#include "zlibng_wrapper.h"

Error write_idat_chunks(UserIO *user_io, const PNGWriteConfig *config, PNG_InternalState *state) {
    const u32 row_size = state_bytes_per_row(state);
    u8 *compressed_buffer;
    u32 compressed_size;
    s32 res = zlibng_compress_buffer(state->scratch_buffer, ((row_size + 1) * state->ihdr.height),
                                     &compressed_buffer, &compressed_size, config->compression_level);
    free(state->scratch_buffer);
    state->scratch_buffer = NULL;
    state->scratch_buffer_size = 0;
    if (res != Z_OK) {
        state_free(state);
        return error_fatal_fmt("Failed to compress PNG image data, zlib-ng error code: %d", res);
    }

    PNGChunk idat_chunk = {0};
    idat_chunk.type = PNG_FOURCC('I', 'D', 'A', 'T');
    Error error;
    if (config->split_idat_chunks) {
        s64 bytes_remaining = compressed_size;
        u32 offset = 0;
        while (bytes_remaining > 0) {
            u32 chunk_size = bytes_remaining > config->split_idat_max_size
                                 ? config->split_idat_max_size
                                 : bytes_remaining;
            idat_chunk.length = chunk_size;
            idat_chunk.data = compressed_buffer + offset;
            idat_chunk.crc = png_compute_crc(&idat_chunk);
            error = png_write_chunk(user_io, &idat_chunk);
            RETURN_IF_FATAL_WITH_CLEANUP(error, {
                                         free(compressed_buffer);
                                         state_free(state);
                                         });

            bytes_remaining -= chunk_size;
            offset += chunk_size;
        }
    } else {
        idat_chunk.length = compressed_size;
        idat_chunk.data = compressed_buffer;
        idat_chunk.crc = png_compute_crc(&idat_chunk);
        error = png_write_chunk(user_io, &idat_chunk);
        RETURN_IF_FATAL_WITH_CLEANUP(error, {
                                     free(compressed_buffer);
                                     state_free(state);
                                     });
    }

    free(compressed_buffer);
    return NO_ERROR;
}

Error write_palette_chunk(UserIO *user_io, PNG_InternalState *state) {
    if (state->palette != NULL) {
        PNGChunk plte_chunk = {0};
        plte_chunk.type = PNG_FOURCC('P', 'L', 'T', 'E');
        plte_chunk.length = state->palette_color_count * 3;

        plte_chunk.data = (u8 *) malloc(state->palette_color_count * 3);
        if (!plte_chunk.data) {
            return error_fatal("Failed to allocate memory for PLTE chunk data");
        }
        memset(plte_chunk.data, 0, state->palette_color_count * 3);
        bool has_alpha = false;
        for (int i = 0; i < state->palette_color_count; ++i) {
            RGBA color = state->palette[i];
            plte_chunk.data[i * 3 + 0] = color.r;
            plte_chunk.data[i * 3 + 1] = color.g;
            plte_chunk.data[i * 3 + 2] = color.b;
            has_alpha |= color.a < 255;
        }

        plte_chunk.crc = png_compute_crc(&plte_chunk); {
            Error error = png_write_chunk(user_io, &plte_chunk);
            RETURN_IF_FATAL_WITH_CLEANUP(error, {
                                         free(plte_chunk.data);
                                         state_free(state);
                                         });
        }
        if (has_alpha) {
            PNGChunk trns_chunk = {0};
            trns_chunk.type = PNG_FOURCC('t', 'R', 'N', 'S');
            trns_chunk.length = state->palette_color_count;
            trns_chunk.data = (u8 *) malloc(state->palette_color_count);
            if (!trns_chunk.data) {
                free(plte_chunk.data);
                return error_fatal("Failed to allocate memory for tRNS chunk data");
            }
            memset(trns_chunk.data, 255, state->palette_color_count);
            for (int i = 0; i < state->palette_color_count; ++i) {
                RGBA color = state->palette[i];
                trns_chunk.data[i] = color.a;
            }
            trns_chunk.crc = png_compute_crc(&trns_chunk);
            Error error = png_write_chunk(user_io, &trns_chunk);
            RETURN_IF_FATAL_WITH_CLEANUP(error, {
                                         free(plte_chunk.data);
                                         }
            );

            free(trns_chunk.data);
        }
    }
    return NO_ERROR;
}

Error filter_image(PNG_InternalState *state) {

    const u32 row_size = state_bytes_per_row(state);
    state->scratch_buffer_size = (row_size + 1) * state->ihdr.height + 1024;
    state->scratch_buffer = (u8 *) malloc(state->scratch_buffer_size);
    if (!state->scratch_buffer) {
        return error_fatal("Failed to allocate memory for PNG image data");
    }

    state->compressed_buffer_size = 0;
    state->compressed_buffer = NULL; // Not used here

    u8 *row_buffer = malloc(row_size + 1);
    if (!row_buffer) {
        return error_fatal("Failed to allocate memory for PNG row buffer");
    }
    u8 *filter_row_buffers = malloc(PNG_FILTER_TYPE_COUNT * row_size);
    u8 *tmp_buffers[PNG_FILTER_TYPE_COUNT] = {0};
    memset(filter_row_buffers, 0, PNG_FILTER_TYPE_COUNT * row_size);
    if (!filter_row_buffers) {
        free(row_buffer);
        return error_fatal("Failed to allocate memory for PNG filter temporary buffers");
    }
    for (u32 i = 0; i < PNG_FILTER_TYPE_COUNT; ++i) {
        tmp_buffers[i] = filter_row_buffers + i * row_size;
    }

    for (u32 y = 0; y < state->ihdr.height; ++y) {
        const u8 *curr_row = state->image_data + y * row_size;
        const u8 *prev_row = (y > 0) ? (state->image_data + (y - 1) * row_size) : NULL;
        const Error filter_error = filter_row(state, curr_row, prev_row, row_buffer, row_size + 1,
                                              tmp_buffers);
        RETURN_IF_FATAL_WITH_CLEANUP(filter_error,
                                     {
                                     free(filter_row_buffers);
                                     free(row_buffer);
                                     free(state->scratch_buffer);
                                     state->scratch_buffer=NULL;
                                     }
        );
        memcpy(state->scratch_buffer + y * (row_size + 1), row_buffer, row_size + 1);
    }
    free(row_buffer);
    free(filter_row_buffers);

    return NO_ERROR;
}

Error png_write(UserIO *user_io, const PNGWriteConfig *config, const PNGFile *png) {
    if (png->image_data == NULL) {
        return error_fatal("PNG image data is NULL");
    }

    PNG_InternalState state = {0};
    state.ihdr.width = png->width;
    state.ihdr.height = png->height;
    state.ihdr.bit_depth = png->bit_depth;
    state.ihdr.color_type = png->color_type;
    state.ihdr.compression_method = 0;
    state.ihdr.filter_method = 0;
    state.ihdr.interlace_method = 0;

    if (state_pixel_size(&state) == 0) {
        return error_fatal("Unsupported PNG color type");
    }

    // Allocate and copy png data
    const size_t bytes_per_row = state_bytes_per_row(&state);
    state.image_data_size = png->height * bytes_per_row;
    state.image_data = malloc(state.image_data_size);
    memcpy(state.image_data, png->image_data, state.image_data_size);

    if (png->palette != NULL) {
        if (png->color_type != PNG_COLOR_TYPE_INDEXED_COLOR) {
            return error_fatal("PNG palette provided but PNG color type is not indexed color");
        }
        state.palette_color_count = png->palette_color_count;
        state.palette = malloc(state.palette_color_count * sizeof(u32));
        memcpy(state.palette, png->palette, state.palette_color_count * sizeof(u32));
    } else if (config->scan_palette && png->color_type != PNG_COLOR_TYPE_INDEXED_COLOR) {
        Error palette_error = png_generate_palette(&state);
        RETURN_IF_FATAL_WITH_CLEANUP(palette_error,
                                     {
                                     state_free(&state);
                                     });
    }

    if (config->strip_empty_alpha &&
        (state.ihdr.color_type == PNG_COLOR_TYPE_GRAYSCALE_WITH_ALPHA ||
         state.ihdr.color_type == PNG_COLOR_TYPE_TRUECOLOR_WITH_ALPHA)) {
        if (state.palette == NULL) {
            const u32 pixel_size = state_pixel_size(&state);
            if (state.ihdr.bit_depth == 8) {
                bool has_alpha = false;
                const u32 pixel_count = state.ihdr.width * state.ihdr.height;
                for (int i = 0; i < pixel_count; ++i) {
                    const u8 *pixel = state.image_data + i * pixel_size;
                    if (pixel_size == 2) {
                        // Gray + Alpha
                        if (pixel[1] < 255) {
                            has_alpha = true;
                            break;
                        }
                    } else if (pixel_size == 4) {
                        // RGBA
                        if (pixel[3] < 255) {
                            has_alpha = true;
                            break;
                        }
                    }
                }
                if (!has_alpha) {
                    // Strip alpha channel
                    u8 *new_image_data = malloc(state.ihdr.width * state.ihdr.height * (pixel_size - 1));
                    for (u32 y = 0; y < state.ihdr.height; ++y) {
                        for (u32 x = 0; x < state.ihdr.width; ++x) {
                            const u8 *src_pixel = state.image_data + (y * state.ihdr.width + x) * pixel_size;
                            u8 *dst_pixel = new_image_data + (y * state.ihdr.width + x) * (pixel_size - 1);
                            memcpy(dst_pixel, src_pixel, pixel_size - 1);
                        }
                    }
                    free(state.image_data);
                    state.image_data = new_image_data;
                    state.image_data_size = state.ihdr.width * state.ihdr.height * (pixel_size - 1);
                    if (state.ihdr.color_type == PNG_COLOR_TYPE_GRAYSCALE_WITH_ALPHA) {
                        state.ihdr.color_type = PNG_COLOR_TYPE_GRAYSCALE;
                    } else if (state.ihdr.color_type == PNG_COLOR_TYPE_TRUECOLOR_WITH_ALPHA) {
                        state.ihdr.color_type = PNG_COLOR_TYPE_TRUECOLOR;
                    }
                }
            }
        }
    }

    if (config->detect_gray) {
        if (state.palette != NULL) {
            if (state.palette_color_count > 64) {
                bool all_gray = true;
                bool has_alpha = false;
                for (int i = 0; i < state.palette_color_count; ++i) {
                    u8 c0 = state.palette[i].r;
                    if (c0 != state.palette[i].g || c0 != state.palette[i].b) {
                        all_gray = false;
                        break;
                    }
                    has_alpha |= state.palette[i].a < 255;
                }
                if (all_gray) {
                    u32 new_pixel_size = has_alpha ? 2 : 1;
                    u8 *new_image_data = malloc(state.ihdr.width * state.ihdr.height * new_pixel_size);
                    for (u32 p = 0; p < state.ihdr.height * state.ihdr.width; ++p) {
                        u8 index = state.image_data[p];
                        u8 gray = state.palette[index].r;
                        if (has_alpha) {
                            u8 alpha = state.palette[index].a;
                            new_image_data[p * 2 + 0] = gray;
                            new_image_data[p * 2 + 1] = alpha;
                        } else {
                            new_image_data[p] = gray;
                        }
                    }
                    free(state.image_data);
                    state.image_data = new_image_data;
                    state.image_data_size = state.ihdr.width * state.ihdr.height * new_pixel_size;
                    free(state.palette);
                    state.palette = NULL;
                    state.palette_color_count = 0;
                    if (has_alpha) {
                        state.ihdr.color_type = PNG_COLOR_TYPE_GRAYSCALE_WITH_ALPHA;
                    } else {
                        state.ihdr.color_type = PNG_COLOR_TYPE_GRAYSCALE;
                    }
                }
            }
        } else {
            const u32 pixel_size = state_pixel_size(&state);
            if (pixel_size == 3 || pixel_size == 4) {
                bool all_gray = true;
                const u32 pixel_count = state.ihdr.width * state.ihdr.height;
                for (int i = 0; i < pixel_count; ++i) {
                    const u8 *pixel = state.image_data + i * pixel_size;
                    if (pixel[0] != pixel[1] || pixel[0] != pixel[2]) {
                        all_gray = false;
                        break;
                    }
                }
                if (all_gray) {
                    u32 new_pixel_size = (pixel_size == 4) ? 2 : 1;
                    u8 *new_image_data = malloc(state.ihdr.width * state.ihdr.height * new_pixel_size);
                    for (u32 y = 0; y < state.ihdr.height; ++y) {
                        for (u32 x = 0; x < state.ihdr.width; ++x) {
                            const u8 *src_pixel = state.image_data + (y * state.ihdr.width + x) * pixel_size;
                            u8 *dst_pixel = new_image_data + (y * state.ihdr.width + x) * new_pixel_size;
                            dst_pixel[0] = src_pixel[0];
                            if (new_pixel_size == 2) {
                                dst_pixel[1] = src_pixel[3];
                            }
                        }
                    }
                    free(state.image_data);
                    state.image_data = new_image_data;
                    state.image_data_size = state.ihdr.width * state.ihdr.height * new_pixel_size;
                    if (new_pixel_size == 2) {
                        state.ihdr.color_type = PNG_COLOR_TYPE_GRAYSCALE_WITH_ALPHA;
                    } else {
                        state.ihdr.color_type = PNG_COLOR_TYPE_GRAYSCALE;
                    }
                }
            }
        }
    }
    filter_image(&state);
    {
        RETURN_IF_FATAL_WITH_CLEANUP(write_u64le(user_io, PNG_IDENT), {state_free(&state);});

        u8 ihdr_buffer[13];
        IHDRChunk_to_bytes(&state.ihdr, ihdr_buffer, sizeof(ihdr_buffer));
        PNGChunk ihdr_chunk = {0};
        ihdr_chunk.type = PNG_FOURCC('I', 'H', 'D', 'R');
        ihdr_chunk.length = sizeof(ihdr_buffer);
        ihdr_chunk.data = ihdr_buffer;
        ihdr_chunk.crc = png_compute_crc(&ihdr_chunk);

        RETURN_IF_FATAL_WITH_CLEANUP(png_write_chunk(user_io, &ihdr_chunk), {state_free(&state);});

        write_palette_chunk(user_io, &state);

        write_idat_chunks(user_io, config, &state);

        PNGChunk iend_chunk = {0};
        iend_chunk.type = PNG_FOURCC('I', 'E', 'N', 'D');
        iend_chunk.length = 0;
        iend_chunk.data = NULL;
        iend_chunk.crc = png_compute_crc(&iend_chunk);

        RETURN_IF_FATAL_WITH_CLEANUP(png_write_chunk(user_io, &iend_chunk), {state_free(&state);});
    }

    state_free(&state);

    return NO_ERROR;
}
