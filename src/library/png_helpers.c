// Created by RED on 10.12.2025.

#include "png_helpers.h"
#include "file_utils.h"
#include "png_crc.h"


size_t state_bytes_per_row(const PNG_InternalState *state) {
    size_t channel_count = 0;
    switch (state->ihdr.color_type) {
        case PNG_COLOR_TYPE_GRAYSCALE:
            channel_count = 1;
            break;
        case PNG_COLOR_TYPE_TRUECOLOR:
            channel_count = 3;
            break;
        case PNG_COLOR_TYPE_INDEXED_COLOR:
            channel_count = 1;
            break;
        case PNG_COLOR_TYPE_GRAYSCALE_WITH_ALPHA:
            channel_count = 2;
            break;
        case PNG_COLOR_TYPE_TRUECOLOR_WITH_ALPHA:
            channel_count = 4;
            break;
        default:
            channel_count = 0; // Unknown color type
            break;
    }
    return ((state->ihdr.width * state->ihdr.bit_depth * channel_count + 7) / 8);
}

u32 state_pixel_size(const PNG_InternalState *state) {
    size_t channel_count = 0;
    switch (state->ihdr.color_type) {
        case PNG_COLOR_TYPE_GRAYSCALE:
            channel_count = 1;
            break;
        case PNG_COLOR_TYPE_TRUECOLOR:
            channel_count = 3;
            break;
        case PNG_COLOR_TYPE_INDEXED_COLOR:
            channel_count = 1;
            break;
        case PNG_COLOR_TYPE_GRAYSCALE_WITH_ALPHA:
            channel_count = 2;
            break;
        case PNG_COLOR_TYPE_TRUECOLOR_WITH_ALPHA:
            channel_count = 4;
            break;
        default:
            channel_count = 0; // Unknown color type
            break;
    }
    return PNG_MAX(1, state->ihdr.bit_depth * channel_count / 8);
}

u32 state_channels(const PNG_InternalState *png) {
    switch (png->ihdr.color_type) {
        case PNG_COLOR_TYPE_GRAYSCALE:
            return 1;
        case PNG_COLOR_TYPE_TRUECOLOR:
            return 3;
        case PNG_COLOR_TYPE_INDEXED_COLOR:
            return 1;
        case PNG_COLOR_TYPE_GRAYSCALE_WITH_ALPHA:
            return 2;
        case PNG_COLOR_TYPE_TRUECOLOR_WITH_ALPHA:
            return 4;
        default:
            return 0; // Unknown color type
    }
}

Error read_png_chunk(UserIO *user_io, PNGChunk *chunk) {
    RETURN_IF_FATAL_WITH_CLEANUP(read_u32be(user_io, &chunk->length), {});
    RETURN_IF_FATAL_WITH_CLEANUP(read_u32le(user_io, &chunk->type), {});
    // Read in original byte order for CRC calculation

    if (chunk->length > 0) {
        chunk->data = (u8 *) malloc(chunk->length);
        if (!chunk->data) {
            return error_fatal("Failed to allocate memory for PNG chunk data");
        }

        RETURN_IF_FATAL_WITH_CLEANUP(read_bytes(user_io, chunk->data, chunk->length), {
                                     free(chunk->data);
                                     });
    } else {
        chunk->data = NULL;
    }

    RETURN_IF_FATAL_WITH_CLEANUP(read_u32be(user_io, &chunk->crc), {});

    if (chunk->length>0) {
        u32 computed_crc = png_update_crc(0xFFFFFFFF, (u8 *) &chunk->type, 4);
        computed_crc = png_update_crc(computed_crc, chunk->data, chunk->length);
        computed_crc ^= 0xffffffff;
        if (computed_crc != chunk->crc) {
            free(chunk->data);
            return error_fatal_fmt("PNG chunk CRC mismatch, expected %08X, got %08X", chunk->crc, computed_crc);
        }
    }

    return NO_ERROR;
}

void print_chunk(const PNGChunk *chunk) {
    printf("Chunk Type: %.4s, Length: %u, CRC: %08X\n", (char *) &chunk->type, chunk->length, chunk->crc);
}

u32 png_compute_crc(const PNGChunk *chunk) {
    u32 crc = png_update_crc(0xFFFFFFFF, (u8 *) &chunk->type, 4);
    crc = png_update_crc(crc, chunk->data, chunk->length);
    crc ^= 0xffffffff;
    return crc;
}

Error png_write_chunk(UserIO *user_io, const PNGChunk *chunk) {
    RETURN_IF_FATAL_WITH_CLEANUP(write_u32be(user_io, chunk->length), {});
    RETURN_IF_FATAL_WITH_CLEANUP(write_u32le(user_io, chunk->type), {}); // Write in original byte order for CRC calculation
    if (chunk->length > 0) {
        RETURN_IF_FATAL_WITH_CLEANUP(write_bytes(user_io, chunk->data, chunk->length), {});
    }

    RETURN_IF_FATAL_WITH_CLEANUP(write_u32be(user_io, chunk->crc), {});

    return NO_ERROR;
}

void state_free(PNG_InternalState *state) {
    if (state->scratch_buffer) {
        free(state->scratch_buffer);
        state->scratch_buffer = NULL;
    }
    if (state->compressed_buffer) {
        free(state->compressed_buffer);
        state->compressed_buffer = NULL;
    }
    if (state->image_data) {
        free(state->image_data);
        state->image_data = NULL;
    }
    if (state->palette) {
        free(state->palette);
        state->palette = NULL;
        state->palette_color_count = 0;
    }

    if (state->srgb) {
        free(state->srgb);
        state->srgb = NULL;
    }

    if (state->gama) {
        free(state->gama);
        state->gama = NULL;
    }

    if (state->chrm) {
        free(state->chrm);
        state->chrm = NULL;
    }

    if (state->phys) {
        free(state->phys);
        state->phys = NULL;
    }

    if (state->text) {
        for (u32 i = 0; i < state->text_chunk_count; ++i) {
            tExtChunk_free(&state->text[i]);
        }
        free(state->text);
        state->text = NULL;
        state->text_chunk_count = 0;
    }

    if (state->itxt) {
        iTxtChunk_free(state->itxt);
        free(state->itxt);
        state->itxt = NULL;
    }

    if (state->iccp) {
        iCCPChunk_free(state->iccp);
        free(state->iccp);
        state->iccp = NULL;
    }
}
