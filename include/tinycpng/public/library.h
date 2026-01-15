#pragma once
#include <stdio.h>

#include "error_utils.h"
#include "file_utils.h"
#include "mytypes.h"

#if defined(TINYCPNG_EXPORTS)
#if defined(__GNUC__) || defined(__clang__)
#define PNG_DL_EXPORT __attribute__((visibility("default")))
#elif defined(_MSC_VER)
#define PNG_DL_EXPORT  __declspec(dllexport)
#else
#define PNG_DL_EXPORT
#endif
#define PNG_API PNG_DL_EXPORT
#elif defined(TINYCPNG_IMPORT)
#if defined(__GNUC__) || defined(__clang__)
#define PNG_DL_EXPORT __attribute__((visibility("default")))
#elif defined(_MSC_VER)
#define PNG_DL_EXPORT  __declspec(dllimport)
#else
#define PNG_DL_EXPORT
#endif
#define PNG_API PNG_DL_EXPORT
#else
#define PNG_DL_EXPORT
#define PNG_API PNG_DL_EXPORT
#endif

// PNG ident  137 80 78 71 13 10 26 10
static const u64 PNG_IDENT = 137 | 80 << 8 | 78 << 16 | 71 << 24 | 13ULL << 32 | 10ULL << 40 | 26ULL << 48 | 10ULL <<
                             56;
#ifndef MAX_PNG_MEMORY
#define MAX_PNG_MEMORY (512*1024*1024)
#endif

typedef struct {
    u32 length;
    u32 type;
    u8 *data;
    u32 crc;
} PNGChunk;

void PNGChunk_free(PNGChunk *chunk);

typedef enum {
    PNG_COLOR_TYPE_GRAYSCALE = 0,
    PNG_COLOR_TYPE_TRUECOLOR = 2,
    PNG_COLOR_TYPE_INDEXED_COLOR = 3,
    PNG_COLOR_TYPE_GRAYSCALE_WITH_ALPHA = 4,
    PNG_COLOR_TYPE_TRUECOLOR_WITH_ALPHA = 6
} PNGColorType;

typedef enum {
    PNG_FILTER_TYPE_NONE = 0,
    PNG_FILTER_TYPE_SUB = 1,
    PNG_FILTER_TYPE_UP = 2,
    PNG_FILTER_TYPE_AVERAGE = 3,
    PNG_FILTER_TYPE_PAETH = 4,

    PNG_FILTER_TYPE_COUNT = 5
} PNGFilterType;

typedef enum {
    SRGB_RENDERING_INTENT_PERCEPTUAL = 0,
    SRGB_RENDERING_INTENT_RELATIVE_COLORIMETRIC = 1,
    SRGB_RENDERING_INTENT_SATURATION = 2,
    SRGB_RENDERING_INTENT_ABSOLUTE_COLORIMETRIC = 3
} PNGRenderingIntent;

typedef enum {
    PNG_UNIT_SPECIFIER_UNKNOWN = 0,
    PNG_UNIT_SPECIFIER_METER = 1
} PNGUnitSpecifier;

typedef struct {
    const char *keyword;
    const char *text;
} PNGTextBlock;

typedef struct {
    f32 white_point_x;
    f32 white_point_y;
    f32 red_x;
    f32 red_y;
    f32 green_x;
    f32 green_y;
    f32 blue_x;
    f32 blue_y;
} PNGChromaticities;

typedef struct {
    u32 pixels_per_unit_x;
    u32 pixels_per_unit_y;
    PNGUnitSpecifier unit_specifier;
} PNGPhysicalDimensions;

typedef struct {
    const char *keyword;
    u8 compression_flag;
    u8 compression_method;
    const u8 *language_tag;
    const u8 *translated_keyword;
    const char *text;
} PNGInternationalTextChunk;

typedef struct {
    const char *profile_name;
    u8 compression_method;
    const u8 *compressed_profile_data;
    u32 compressed_profile_size;
} PNGICCProfileChunk;

typedef struct {
    bool split_idat_chunks;
    u32 split_idat_max_size;
    s32 compression_level;
    bool scan_palette;
    bool strip_empty_alpha;
    bool detect_gray;
} PNGWriteConfig;

// Compression levels compatible with zlib/ng
#define PNG_NO_COMPRESSION         0
#define PNG_BEST_SPEED             1
#define PNG_BEST_COMPRESSION       9
#define PNG_DEFAULT_COMPRESSION  (-1)

PNG_API void PNGWriteConfig_default(PNGWriteConfig *config);


typedef struct {
    u32 width;
    u32 height;
    u8 bit_depth;
    PNGColorType color_type;
    u8 *image_data;

    u8 *palette;
    u32 palette_color_count;

    bool has_srgb;
    PNGRenderingIntent srgb_intent;

    bool has_gama;
    f32 gamma;

    bool has_chromaticities;
    PNGChromaticities chromaticities;

    bool has_physical_dimensions;
    PNGPhysicalDimensions physical_dimensions;

    bool has_text;
    PNGTextBlock *text_blocks;
    u32 text_block_count;

    bool has_international_text;
    PNGInternationalTextChunk international_text;

    bool has_icc_profile;
    PNGICCProfileChunk icc_profile;
} PNGFile;


PNG_API Error png_read(UserIO *user_io, PNGFile *png);

PNG_API Error png_write(UserIO *user_io, const PNGWriteConfig *config, const PNGFile *png);

PNG_API Error png_from_data(const u8 *data, size_t data_size, u32 width, u32 height, u32 channels, u32 bit_depth,
                            PNGFile *png);

PNG_API Error png_apply_palette(PNGFile *png);

PNG_API u32 png_channels(const PNGFile *png);

PNG_API void png_free(PNGFile *png);
