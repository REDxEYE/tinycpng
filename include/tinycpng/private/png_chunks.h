// Created by RED on 11.12.2025.
#pragma once
#include "library.h"
#include "mytypes.h"

typedef struct {
    u32 width;
    u32 height;
    u8 bit_depth;
    PNGColorType color_type;
    u8 compression_method;
    u8 filter_method;
    u8 interlace_method;
} IHDRChunk;

Error IHDRChunk_from_bytes(IHDRChunk *chunk, const u8 *data);
Error IHDRChunk_to_bytes(const IHDRChunk *chunk, u8* buffer, u32 buffer_size);

typedef struct {
    PNGRenderingIntent rendering_intent;
} sRGBChunk;

Error sRGBChunk_from_bytes(sRGBChunk *chunk, const u8 *data);

typedef struct {
    u32 gamma_fixed_point; // gamma value multiplied by 100000
} gAMAChunk;

Error gAMAChunk_from_bytes(gAMAChunk *chunk, const u8 *data);

typedef struct {
    u32 white_point_x; // value multiplied by 100000
    u32 white_point_y; // value multiplied by 100000
    u32 red_x; // value multiplied by 100000
    u32 red_y; // value multiplied by 100000
    u32 green_x; // value multiplied by 100000
    u32 green_y; // value multiplied by 100000
    u32 blue_x; // value multiplied by 100000
    u32 blue_y; // value multiplied by 100000
} cHRMChunk;

Error cHRMChunk_from_bytes(cHRMChunk *chunk, const u8 *data);


typedef struct {
    u32 pixels_per_unit_x;
    u32 pixels_per_unit_y;
    PNGUnitSpecifier unit_specifier;
} pHYsChunk;

Error pHYsChunk_from_bytes(pHYsChunk *chunk, const u8 *data);

typedef struct {
    const char *keyword;
    const char *text;
} tEXtChunk;

Error tEXtChunk_from_bytes(tEXtChunk *chunk, const u8 *data, u32 length);

void tExtChunk_free(tEXtChunk *chunk);

typedef struct {
    const char *keyword;
    u8 compression_flag;
    u8 compression_method;
    u8 *language_tag;
    u8 *translated_keyword;
    const char *text;
} iTXtChunk;

Error iTXtChunk_from_bytes(iTXtChunk *chunk, const u8 *data, u32 length);
void iTxtChunk_free(iTXtChunk *chunk);

typedef struct {
    const char* profile_name;
    u8 compression_method;
    u8 *compressed_profile_data;
    u32 compressed_profile_size;
}iCCPChunk;

Error iCCPChunk_from_bytes(iCCPChunk *chunk, const u8 *data, u32 length);
void iCCPChunk_free(iCCPChunk *chunk);