// Created by RED on 10.12.2025.
#pragma once
#include <stdbool.h>
#include <stdlib.h>

#include "error_utils.h"
#include "mytypes.h"
#include "library.h"
#include "png_chunks.h"

#define PNG_FOURCC(a, b, c, d) ((u32)(a) | (u32)(b) << 8 | (u32)(c) << 16 | (u32)(d)<< 24)

typedef struct {
    bool ihdr_read;
    bool iend_read;

    u8 *scratch_buffer;
    size_t scratch_buffer_size;

    u8 *compressed_buffer;
    size_t compressed_buffer_size;
    size_t compressed_buffer_offset;

    u8* image_data;
    u64 image_data_size;

    RGBA* palette;
    u32 palette_color_count;

    IHDRChunk ihdr;

    sRGBChunk *srgb;
    gAMAChunk *gama;
    cHRMChunk *chrm;
    pHYsChunk *phys;
    tEXtChunk *text;
    u32 text_chunk_count;
    iTXtChunk *itxt;
    iCCPChunk *iccp;


} PNG_InternalState;


size_t state_bytes_per_row(const PNG_InternalState *state);
u32 state_pixel_size(const PNG_InternalState *state);
u32 state_channels(const PNG_InternalState *png);

Error read_png_chunk(UserIO *user_io, PNGChunk *chunk);

void print_chunk(const PNGChunk *chunk);

u32 png_compute_crc(const PNGChunk *chunk);

Error png_write_chunk(UserIO *user_io, const PNGChunk *chunk);

void state_free(PNG_InternalState *state);

