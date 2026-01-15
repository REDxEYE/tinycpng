// Created by RED on 15.12.2025.

#include "png_read.h"

#include <string.h>
#include <sys/stat.h>

#include "file_utils.h"
#include "png_filters.h"
#include "png_helpers.h"
#include "zlib-ng.h"
#include "zlibng_wrapper.h"

Error png_read(UserIO *user_io, PNGFile *png) {
    u64 ident = 0;
    RETURN_IF_FATAL_WITH_CLEANUP(read_u64le(user_io, &ident), {});

    if (ident != PNG_IDENT) {
        return error_fatal_fmt("Invalid png ident, expected %08X, but got %08X", PNG_IDENT, ident);
    }

    PNG_InternalState state = {0};

    while (true) {
        PNGChunk chunk;
        RETURN_IF_FATAL_WITH_CLEANUP(read_png_chunk(user_io, &chunk), {
                                     state_free(&state);
                                     });


        switch (chunk.type) {
            case PNG_FOURCC('I', 'H', 'D', 'R'): {
                if (state.ihdr_read) {
                    PNGChunk_free(&chunk);
                    state_free(&state);
                    return error_fatal("Multiple IHDR chunks found");
                }

                if (chunk.length != 13) {
                    PNGChunk_free(&chunk);
                    state_free(&state);
                    return error_fatal_fmt(" Invalid IHDR chunk size, expected %u, but got %u", 13, chunk.length);
                }
                IHDRChunk_from_bytes(&state.ihdr, chunk.data);
                PNGChunk_free(&chunk);

                if (state.ihdr.filter_method != 0) {
                    state_free(&state);
                    return error_fatal_fmt("Unsupported PNG filter method %i", state.ihdr.filter_method);
                }

                if (state.ihdr.interlace_method != 0) {
                    state_free(&state);
                    return error_fatal("Interlaced PNGs are not supported");
                }

                state.ihdr_read = true;

                const size_t bytes_per_row = state_bytes_per_row(&state);
                if (bytes_per_row == 0) {
                    state_free(&state);
                    return error_fatal("Unsupported PNG color type");
                }
                const size_t scratch_buffer_size = (bytes_per_row + 1) * state.ihdr.height + 1024;
                // +1 for filter byte per row, +1024 for some safety margin

                if (scratch_buffer_size > MAX_PNG_MEMORY) {
                    state_free(&state);
                    return error_fatal("PNG image size exceeds maximum allowed memory");
                }

                if (scratch_buffer_size == 0) {
                    state_free(&state);
                    return error_fatal("Invalid PNG dimensions resulting in zero image size");
                }
                state.scratch_buffer = (u8 *) malloc(scratch_buffer_size);
                if (!state.scratch_buffer) {
                    state_free(&state);
                    return error_fatal("Failed to allocate memory for PNG image data");
                }
                state.scratch_buffer_size = scratch_buffer_size;

                state.compressed_buffer = (u8 *) malloc(scratch_buffer_size);
                if (!state.compressed_buffer) {
                    state_free(&state);
                    return error_fatal("Failed to allocate memory for PNG image data");
                }
                state.compressed_buffer_size = scratch_buffer_size;

                break;
            }

            case PNG_FOURCC('I', 'D', 'A', 'T'): {
                const u8 *raw_data = chunk.data;
                const u32 raw_data_len = chunk.length;

                if (state.compressed_buffer_offset + raw_data_len > state.compressed_buffer_size) {
                    size_t new_size = state.compressed_buffer_size * 2;
                    if (new_size < state.compressed_buffer_offset + raw_data_len)
                        new_size = state.compressed_buffer_offset + raw_data_len;

                    u8 *tmp = realloc(state.compressed_buffer, new_size);
                    if (!tmp) {
                        PNGChunk_free(&chunk);
                        state_free(&state);
                        return error_fatal("Failed to grow PNG compressed buffer");
                    }
                    state.compressed_buffer = tmp;
                    state.compressed_buffer_size = new_size;
                }

                memcpy(state.compressed_buffer + state.compressed_buffer_offset, raw_data, raw_data_len);
                state.compressed_buffer_offset += raw_data_len;
                PNGChunk_free(&chunk);
                break;
            }

            case PNG_FOURCC('P', 'L', 'T', 'E'): {
                if (state.palette != NULL) {
                    PNGChunk_free(&chunk);
                    state_free(&state);
                    return error_fatal("Multiple PLTE chunks found");
                }
                const u32 palette_max_color_count = 1 << state.ihdr.bit_depth;
                const u32 actual_palette_color_count = chunk.length / 3;
                if (actual_palette_color_count > palette_max_color_count) {
                    PNGChunk_free(&chunk);
                    state_free(&state);
                    return error_fatal_fmt("PLTE chunk has too many colors, maximum for bit depth %u is %u, but got %u",
                                           state.ihdr.bit_depth, palette_max_color_count, actual_palette_color_count);
                }
                state.palette = malloc(palette_max_color_count * sizeof(RGBA));
                if (state.palette == NULL) {
                    PNGChunk_free(&chunk);
                    state_free(&state);
                    return error_fatal("Failed to allocate memory for PNG palette");
                }
                memset(state.palette, 255, palette_max_color_count * sizeof(RGBA));

                for (int i = 0; i < 1 << state.ihdr.bit_depth; ++i) {
                    memcpy(&state.palette[i], chunk.data + i * 3, 3);
                }
                break;
            }

            case PNG_FOURCC('I', 'E', 'N', 'D'): {
                if (chunk.length != 0 && chunk.data != NULL) {
                    PNGChunk_free(&chunk);
                    state_free(&state);
                    return error_fatal("IEND chunk must not have any data");
                }
                state.iend_read = true;
                break;
            }

            case PNG_FOURCC('s', 'R', 'G', 'B'): {
                if (state.srgb != NULL) {
                    PNGChunk_free(&chunk);
                    state_free(&state);
                    return error_fatal("Multiple sRGB chunks found");
                }
                if (chunk.length != 1) {
                    PNGChunk_free(&chunk);
                    state_free(&state);
                    return error_fatal_fmt("Invalid sRGB chunk size, expected %u, but got %u", 1, chunk.length);
                }
                state.srgb = (sRGBChunk *) malloc(sizeof(sRGBChunk));
                if (!state.srgb) {
                    PNGChunk_free(&chunk);
                    state_free(&state);
                    return error_fatal("Failed to allocate memory for sRGB chunk");
                }
                RETURN_IF_FATAL_WITH_CLEANUP(sRGBChunk_from_bytes(state.srgb, chunk.data), {
                                             PNGChunk_free(&chunk);
                                             state_free(&state);
                                             });
                PNGChunk_free(&chunk);
                break;
            }

            case PNG_FOURCC('g', 'A', 'M', 'A'): {
                if (state.gama != NULL) {
                    PNGChunk_free(&chunk);
                    state_free(&state);
                    return error_fatal("Multiple gAMA chunks found");
                }
                if (chunk.length != 4) {
                    PNGChunk_free(&chunk);
                    state_free(&state);
                    return error_fatal_fmt("Invalid gAMA chunk size, expected %u, but got %u", 4, chunk.length);
                }
                state.gama = (gAMAChunk *) malloc(sizeof(gAMAChunk));
                if (!state.gama) {
                    PNGChunk_free(&chunk);
                    state_free(&state);
                    return error_fatal("Failed to allocate memory for gAMA chunk");
                }
                RETURN_IF_FATAL_WITH_CLEANUP(gAMAChunk_from_bytes(state.gama, chunk.data), {
                                             PNGChunk_free(&chunk);
                                             state_free(&state);
                                             });
                PNGChunk_free(&chunk);
                break;
            }

            case PNG_FOURCC('c', 'H', 'R', 'M'): {
                if (state.chrm != NULL) {
                    PNGChunk_free(&chunk);
                    state_free(&state);
                    return error_fatal("Multiple cHRM chunks found");
                }
                if (chunk.length != 32) {
                    PNGChunk_free(&chunk);
                    state_free(&state);
                    return error_fatal_fmt("Invalid cHRM chunk size, expected %u, but got %u", 32, chunk.length);
                }
                state.chrm = (cHRMChunk *) malloc(sizeof(cHRMChunk));
                if (!state.chrm) {
                    PNGChunk_free(&chunk);
                    state_free(&state);
                    return error_fatal("Failed to allocate memory for cHRM chunk");
                }
                RETURN_IF_FATAL_WITH_CLEANUP(cHRMChunk_from_bytes(state.chrm, chunk.data), {
                                             PNGChunk_free(&chunk);
                                             state_free(&state);
                                             });
                PNGChunk_free(&chunk);
                break;
            }

            case PNG_FOURCC('p', 'H', 'Y', 's'): {
                if (state.phys != NULL) {
                    PNGChunk_free(&chunk);
                    state_free(&state);
                    return error_fatal("Multiple pHYs chunks found");
                }
                if (chunk.length != 9) {
                    PNGChunk_free(&chunk);
                    state_free(&state);
                    return error_fatal_fmt("Invalid pHYs chunk size, expected %u, but got %u", 9, chunk.length);
                }
                state.phys = (pHYsChunk *) malloc(sizeof(pHYsChunk));
                if (!state.phys) {
                    PNGChunk_free(&chunk);
                    state_free(&state);
                    return error_fatal("Failed to allocate memory for pHYs chunk");
                }
                const Error error = pHYsChunk_from_bytes(state.phys, chunk.data);
                RETURN_IF_FATAL_WITH_CLEANUP(error, {
                                             state_free(&state);
                                             });
                PNGChunk_free(&chunk);
                break;
            }

            case PNG_FOURCC('t', 'E', 'X', 't'): {
                tEXtChunk text_chunk;
                const Error error = tEXtChunk_from_bytes(&text_chunk, chunk.data, chunk.length);
                RETURN_IF_FATAL_WITH_CLEANUP(error, {
                                             state_free(&state);
                                             });
                PNGChunk_free(&chunk);

                tEXtChunk *new_text_chunks = (tEXtChunk *) realloc(
                    state.text,
                    sizeof(tEXtChunk) * (state.text_chunk_count + 1));
                if (!new_text_chunks) {
                    tExtChunk_free(&text_chunk);
                    state_free(&state);
                    return error_fatal("Failed to allocate memory for tEXt chunks");
                }
                state.text = new_text_chunks;
                state.text[state.text_chunk_count] = text_chunk;
                state.text_chunk_count += 1;
                break;
            }

            case PNG_FOURCC('e', 'X', 'I', 'f'): {
                RETURN_IF_FATAL_WITH_CLEANUP(
                    error_warning("eXIf chunk found, but EXIF data is not supported and will be ignored"),
                    {PNGChunk_free(&chunk);}
                );
                PNGChunk_free(&chunk);
                break;
            }

            case PNG_FOURCC('i', 'T', 'X', 't'): {
                if (state.itxt != NULL) {
                    PNGChunk_free(&chunk);
                    state_free(&state);
                    return error_fatal("Multiple iTXt chunks found");
                }
                state.itxt = (iTXtChunk *) malloc(sizeof(iTXtChunk));
                if (!state.itxt) {
                    PNGChunk_free(&chunk);
                    state_free(&state);
                    return error_fatal("Failed to allocate memory for iTXt chunk");
                }
                const Error error = iTXtChunk_from_bytes(state.itxt, chunk.data, chunk.length);
                PNGChunk_free(&chunk);
                RETURN_IF_FATAL_WITH_CLEANUP(error, {
                                             state_free(&state);
                                             });
                break;
            }

            case PNG_FOURCC('i', 'C', 'C', 'P'): {
                if (state.iccp != NULL) {
                    PNGChunk_free(&chunk);
                    state_free(&state);
                    return error_fatal("Multiple iCCP chunks found");
                }
                state.iccp = (iCCPChunk *) malloc(sizeof(iCCPChunk));
                if (!state.iccp) {
                    PNGChunk_free(&chunk);
                    state_free(&state);
                    return error_fatal("Failed to allocate memory for iCCP chunk");
                }
                const Error error = iCCPChunk_from_bytes(state.iccp, chunk.data, chunk.length);
                PNGChunk_free(&chunk);
                RETURN_IF_FATAL_WITH_CLEANUP(error, {
                                             state_free(&state);
                                             });
                break;
            }

            default: {
                printf("Unhandled chunk type: %.4s\n", (char *) &chunk.type);
                print_chunk(&chunk);
                PNGChunk_free(&chunk);
                break;
            }
        }
        PNGChunk_free(&chunk);
        if (state.iend_read) {
            break;
        }
    }

    const s32 res = zlibng_decompress_buffer(state.compressed_buffer, (u32) state.compressed_buffer_offset,
                                             state.scratch_buffer, (u32) state.scratch_buffer_size);
    if (res != Z_OK) {
        state_free(&state);
        return error_fatal_fmt("Failed to decompress PNG image data, zlib-ng error code: %d", res);
    }
    free(state.compressed_buffer);
    state.compressed_buffer = NULL;
    const size_t bytes_per_row = state_bytes_per_row(&state);
    const size_t image_size = state.ihdr.height * bytes_per_row;
    if (image_size > MAX_PNG_MEMORY) {
        state_free(&state);
        return error_fatal("PNG image size exceeds maximum allowed memory");
    }
    state.image_data = (u8 *) malloc(image_size);
    if (!state.image_data) {
        state_free(&state);
        return error_fatal("Failed to allocate memory for final PNG image data");
    }

    const u32 row_size = (bytes_per_row + 1);
    for (int i = 0; i < state.ihdr.height; ++i) {
        const u8 *row = state.scratch_buffer + i * row_size;
        const Error row_error = unfilter_row(&state, row, row_size, state.image_data + i * (row_size - 1),
                                             i > 0 ? state.image_data + (i - 1) * (row_size - 1) : NULL);
        RETURN_IF_FATAL_WITH_CLEANUP(row_error, {
                                     state_free(&state);
                                     });
    }

    png->width = state.ihdr.width;
    png->height = state.ihdr.height;
    png->bit_depth = state.ihdr.bit_depth;
    png->color_type = state.ihdr.color_type;

    png->image_data = state.image_data; // Transfer ownership
    state.image_data = NULL;

    if (state.palette != NULL) {
        png->palette = (u8 *) state.palette;
        state.palette = NULL;
        png->palette_color_count = state.palette_color_count;
    }

    if (state.srgb != NULL) {
        png->has_srgb = true;
        png->srgb_intent = state.srgb->rendering_intent;
    } else {
        png->has_srgb = false;
    }
    if (state.gama != NULL) {
        png->has_gama = true;
        png->gamma = (f32) state.gama->gamma_fixed_point / 100000.0f;
    } else {
        png->has_gama = false;
    }
    if (state.chrm != NULL) {
        png->has_chromaticities = true;
        png->chromaticities.white_point_x = (f32) state.chrm->white_point_x / 100000.0f;
        png->chromaticities.white_point_y = (f32) state.chrm->white_point_y / 100000.0f;
        png->chromaticities.red_x = (f32) state.chrm->red_x / 100000.0f;
        png->chromaticities.red_y = (f32) state.chrm->red_y / 100000.0f;
        png->chromaticities.green_x = (f32) state.chrm->green_x / 100000.0f;
        png->chromaticities.green_y = (f32) state.chrm->green_y / 100000.0f;
        png->chromaticities.blue_x = (f32) state.chrm->blue_x / 100000.0f;
        png->chromaticities.blue_y = (f32) state.chrm->blue_y / 100000.0f;
    } else {
        png->has_chromaticities = false;
    }

    if (state.phys != NULL) {
        png->has_physical_dimensions = true;
        png->physical_dimensions.pixels_per_unit_x = state.phys->pixels_per_unit_x;
        png->physical_dimensions.pixels_per_unit_y = state.phys->pixels_per_unit_y;
        png->physical_dimensions.unit_specifier = state.phys->unit_specifier;
    } else {
        png->has_physical_dimensions = false;
    }

    if (state.text != NULL) {
        png->has_text = true;
        png->text_blocks = (PNGTextBlock *) malloc(sizeof(PNGTextBlock) * state.text_chunk_count);
        if (!png->text_blocks) {
            state_free(&state);
            return error_fatal("Failed to allocate memory for PNG text blocks");
        }
        png->text_block_count = state.text_chunk_count;
        for (u32 i = 0; i < state.text_chunk_count; ++i) {
            png->text_blocks[i].keyword = state.text[i].keyword;
            png->text_blocks[i].text = state.text[i].text;
            state.text[i].keyword = NULL;
            state.text[i].text = NULL;
        }
    } else {
        png->has_text = false;
    }

    if (state.itxt != NULL) {
        png->has_international_text = true;
        // Transfer ownership of iTXt fields
        png->international_text.keyword = state.itxt->keyword;
        png->international_text.compression_flag = state.itxt->compression_flag;
        png->international_text.compression_method = state.itxt->compression_method;
        png->international_text.language_tag = state.itxt->language_tag;
        png->international_text.translated_keyword = state.itxt->translated_keyword;
        png->international_text.text = state.itxt->text;

        state.itxt->keyword = NULL;
        state.itxt->language_tag = NULL;
        state.itxt->translated_keyword = NULL;
        state.itxt->text = NULL;
    } else {
        png->has_international_text = false;
    }

    if (state.iccp != NULL) {
        png->has_icc_profile = true;
        png->icc_profile.profile_name = state.iccp->profile_name;
        png->icc_profile.compression_method = state.iccp->compression_method;
        png->icc_profile.compressed_profile_data = state.iccp->compressed_profile_data;
        png->icc_profile.compressed_profile_size = state.iccp->compressed_profile_size;

        state.iccp->profile_name = NULL;
        state.iccp->compressed_profile_data = NULL;
    } else {
        png->has_icc_profile = false;
    }
    state_free(&state);
    return NO_ERROR;
}
