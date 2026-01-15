#include "library.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "png_helpers.h"
#include "mytypes.h"

void PNGChunk_free(PNGChunk *chunk) {
    if (chunk->data) {
        free(chunk->data);
        chunk->data = NULL;
    }
    chunk->length = 0;
    chunk->type = 0;
    chunk->crc = 0;
}

void PNGWriteConfig_default(PNGWriteConfig *config) {
    config->compression_level = PNG_DEFAULT_COMPRESSION;
    config->split_idat_chunks = false;
    config->split_idat_max_size = 65535;
    config->scan_palette = false;
    config->strip_empty_alpha = false;
    config->detect_gray = false;
}

Error png_from_data(const u8 *data, const size_t data_size, const u32 width, const u32 height, const u32 channels,
                    const u32 bit_depth, PNGFile *png) {
    if (channels != 1 && channels != 2 && channels != 3 && channels != 4) {
        return error_fatal("Unsupported number of channels, must be 1, 2, 3 or 4");
    }
    if (bit_depth != 8 && bit_depth != 16) {
        return error_fatal("TODO: Support non 8/16-bit PNGs");
    }
    if (channels > 1 && bit_depth < 8) {
        return error_fatal("TODO: Support packed PNGs");
    }
    if (width == 0 || height == 0) {
        return error_fatal("Width and height must be greater than 0");
    }
    if (width > 0x7FFFFFFF || height > 0x7FFFFFFF) {
        return error_fatal("Width or height too large");
    }

    png->width = width;
    png->height = height;
    png->bit_depth = (u8) bit_depth;
    png->color_type = (channels == 1)
                          ? PNG_COLOR_TYPE_GRAYSCALE
                          : (channels == 2)
                                ? PNG_COLOR_TYPE_GRAYSCALE_WITH_ALPHA
                                : (channels == 3)
                                      ? PNG_COLOR_TYPE_TRUECOLOR
                                      : PNG_COLOR_TYPE_TRUECOLOR_WITH_ALPHA;
    const size_t bpp = bit_depth * channels;
    const size_t row_size = (size_t) ceil((double) (width * bpp + 7) / 8);
    if (row_size + 1 > 0x7FFFFFFF) {
        return error_fatal("Width is out of PNG spec");
    }

    const size_t image_size = row_size * height;
    if (image_size > MAX_PNG_MEMORY) {
        return error_fatal("PNG image size exceeds maximum allowed memory");
    }
    png->image_data = (u8 *) malloc(image_size);
    if (!png->image_data) {
        return error_fatal("Failed to allocate memory for PNG image data");
    }

    memcpy(png->image_data, data, data_size < image_size ? data_size : image_size);
    return NO_ERROR;
}

Error png_apply_palette(PNGFile *png) {
    if (png->color_type != PNG_COLOR_TYPE_INDEXED_COLOR) {
        return error_warning("PNG is not in indexed color mode");
    }

    if (png->bit_depth > 8) {
        return error_fatal("Unsupported bit depth for indexed PNG");
    }

    if (png->palette == NULL) {
        return error_fatal("Missing palette for indexed PNG");
    }

    bool palette_has_alpha = false;
    for (int i = 0; i < png->palette_color_count; ++i) {
        if (png->palette[i * 4 + 3] < 255) {
            palette_has_alpha = true;
            break;
        }
    }

    u32 pixel_count = png->width * png->height;
    u32 new_pixel_size = 3 + palette_has_alpha; // RGB/A
    u8 *new_image_data = (u8 *) malloc(pixel_count * new_pixel_size);
    if (!new_image_data) {
        return error_fatal("Failed to allocate memory for paletted PNG image data");
    }
    switch (png->bit_depth) {
        case 1: {
            for (u32 i = 0; i < pixel_count / 8; ++i) {
                const u8 index = png->image_data[i];
                for (int j = 7; j >= 0; --j) {
                    const u8 palette_index = (index >> j) & 0x01;
                    new_image_data[i * 8 * 3 + (7 - j) * 3 + 0] = png->palette[palette_index * 4 + 0];
                    new_image_data[i * 8 * 3 + (7 - j) * 3 + 1] = png->palette[palette_index * 4 + 1];
                    new_image_data[i * 8 * 3 + (7 - j) * 3 + 2] = png->palette[palette_index * 4 + 2];
                    if (palette_has_alpha) {
                        new_image_data[i * 8 * 4 + (7 - j) * 4 + 3] = png->palette[palette_index * 4 + 3];
                    }
                }
            }
            break;
        }
        case 2: {
            for (u32 i = 0; i < pixel_count / 4; ++i) {
                const u8 index = png->image_data[i];
                for (int j = 3; j >= 0; --j) {
                    const u8 palette_index = (index >> (j * 2)) & 0x03;
                    new_image_data[i * 4 * 3 + (3 - j) * 3 + 0] = png->palette[palette_index * 4 + 0];
                    new_image_data[i * 4 * 3 + (3 - j) * 3 + 1] = png->palette[palette_index * 4 + 1];
                    new_image_data[i * 4 * 3 + (3 - j) * 3 + 2] = png->palette[palette_index * 4 + 2];
                    if (palette_has_alpha) {
                        new_image_data[i * 4 * 4 + (3 - j) * 4 + 3] = png->palette[palette_index * 4 + 3];
                    }
                }
            }
            break;
        }
        case 4: {
            for (int i = 0; i < pixel_count / 2; ++i) {
                const u8 index = png->image_data[i];
                const u8 index1 = (index >> 4) & 0x0F;
                const u8 index2 = index & 0x0F;

                new_image_data[i * 2 * 3 + 0] = png->palette[index1 * 4 + 0];
                new_image_data[i * 2 * 3 + 1] = png->palette[index1 * 4 + 1];
                new_image_data[i * 2 * 3 + 2] = png->palette[index1 * 4 + 2];

                new_image_data[i * 2 * 3 + 3] = png->palette[index2 * 4 + 0];
                new_image_data[i * 2 * 3 + 4] = png->palette[index2 * 4 + 1];
                new_image_data[i * 2 * 3 + 5] = png->palette[index2 * 4 + 2];
                if (palette_has_alpha) {
                    new_image_data[i * 2 * 4 + 3] = png->palette[index1 * 4 + 3];
                    new_image_data[i * 2 * 4 + 7] = png->palette[index2 * 4 + 3];
                }
            }
            break;
        }
        case 8: {
            for (u32 i = 0; i < pixel_count; ++i) {
                u8 index = png->image_data[i];
                new_image_data[i * 3 + 0] = png->palette[index * 4 + 0];
                new_image_data[i * 3 + 1] = png->palette[index * 4 + 1];
                new_image_data[i * 3 + 2] = png->palette[index * 4 + 2];
                if (palette_has_alpha) {
                    new_image_data[i * 4 + 3] = png->palette[index * 4 + 3];
                }
            }
            break;
        }
        default: {
            return error_fatal("Unsupported bit depth for indexed PNG");
        }
    }

    free(png->image_data);
    png->image_data = new_image_data;
    png->color_type = palette_has_alpha ? PNG_COLOR_TYPE_TRUECOLOR_WITH_ALPHA : PNG_COLOR_TYPE_TRUECOLOR;
    png->bit_depth = 8;

    free(png->palette);
    png->palette = NULL;

    return NO_ERROR;
}

u32 png_channels(const PNGFile *png) {
    switch (png->color_type) {
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

void png_free(PNGFile *png) {
    if (png->image_data) {
        free(png->image_data);
        png->image_data = NULL;
    }
    if (png->palette) {
        free(png->palette);
        png->palette = NULL;
    }
    png->width = 0;
    png->height = 0;
    png->bit_depth = 0;
    png->color_type = 0;

    if (png->has_text) {
        for (u32 i = 0; i < png->text_block_count; ++i) {
            if (png->text_blocks[i].keyword != NULL) {
                free((void *) png->text_blocks[i].keyword);
                png->text_blocks[i].keyword = NULL;
            }
            if (png->text_blocks[i].text != NULL) {
                free((void *) png->text_blocks[i].text);
                png->text_blocks[i].text = NULL;
            }
        }
        free(png->text_blocks);
        png->text_blocks = NULL;
        png->text_block_count = 0;
    }
    png->has_srgb = false;
    png->has_gama = false;
    png->has_chromaticities = false;
    png->has_physical_dimensions = false;

    if (png->has_international_text) {
        if (png->international_text.keyword != NULL) {
            free((void *) png->international_text.keyword);
            png->international_text.keyword = NULL;
        }
        if (png->international_text.language_tag != NULL) {
            free((void *) png->international_text.language_tag);
            png->international_text.language_tag = NULL;
        }
        if (png->international_text.translated_keyword != NULL) {
            free((void *) png->international_text.translated_keyword);
            png->international_text.translated_keyword = NULL;
        }
    }

    if (png->has_icc_profile) {
        if (png->icc_profile.profile_name != NULL) {
            free((void *) png->icc_profile.profile_name);
            png->icc_profile.profile_name = NULL;
        }
        if (png->icc_profile.compressed_profile_data != NULL) {
            free((void *) png->icc_profile.compressed_profile_data);
            png->icc_profile.compressed_profile_data = NULL;
        }
    }
}
