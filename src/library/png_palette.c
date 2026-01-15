// Created by RED on 15.12.2025.
#include "png_palette.h"

Error png_generate_palette_1c8b(PNG_InternalState *state) {
    const u32 max_color = 256;
    u32 color_count = 0;
    RGBA *palette_colors = (RGBA *) malloc(sizeof(RGBA) * max_color);
    memset(palette_colors, 255, sizeof(RGBA) * max_color);
    const u32 num_pixels = state->ihdr.width * state->ihdr.height;
    for (u64 i = 0; i < num_pixels; i++) {
        const u8 pixel = *(u8 *) (state->image_data + i);

        bool color_found = false;
        for (u32 color_id = 0; color_id < color_count; color_id++) {
            if (palette_colors[color_id].r == pixel) {
                color_found = true;
                break;
            }
        }

        if (!color_found) {
            if (color_count >= max_color) {
                free(palette_colors);
                return NO_ERROR;
            }

            palette_colors[color_count].r = pixel;
            palette_colors[color_count].g = pixel;
            palette_colors[color_count].b = pixel;
            color_count++;
        }
    }
    state->palette = palette_colors;
    state->palette_color_count = color_count;
    return NO_ERROR;
}

Error png_generate_palette_2c8b(PNG_InternalState *state) {
    return error_warning("TODO: Palette generation for 2-channel 8-bit PNGs is not implemented yet");
}

Error png_generate_palette_3c8b(PNG_InternalState *state) {
    const u32 max_colors = 256;
    u32 color_count = 0;
    RGBA *palette_colors = (RGBA *) malloc(sizeof(RGBA) * max_colors);
    memset(palette_colors, 255, sizeof(RGBA) * max_colors);
    const u32 num_pixels = state->ihdr.width * state->ihdr.height;
    for (u64 i = 0; i < num_pixels; i++) {
        const RGB pixel = *(RGB *) (state->image_data + i * 3);

        bool color_found = false;
        for (u32 color_id = 0; color_id < color_count; color_id++) {
            if (memcmp(&palette_colors[color_id], &pixel, sizeof(RGB)) == 0) {
                color_found = true;
                break;
            }
        }


        if (!color_found) {
            if (color_count >= max_colors) {
                free(palette_colors);
                return NO_ERROR;
            }
            memcpy(&palette_colors[color_count], &pixel, sizeof(RGB));
            color_count++;
        }
    }
    state->palette = palette_colors;
    state->palette_color_count = color_count;

    return NO_ERROR;
}

Error png_generate_palette_4c8b(PNG_InternalState *state) {
    const u32 max_colors = 256;
    u32 color_count = 0;
    RGBA *palette_colors = (RGBA *) malloc(sizeof(RGBA) * max_colors);
    memset(palette_colors, 255, sizeof(RGBA) * max_colors);
    const u32 num_pixels = state->ihdr.width * state->ihdr.height;

    for (u64 i = 0; i < num_pixels; i++) {
        const RGBA pixel = *(RGBA *) (state->image_data + i * 4);

        bool color_found = false;
        for (u32 color_id = 0; color_id < color_count; color_id++) {
            if (memcmp(&palette_colors[color_id], &pixel, sizeof(RGBA)) == 0) {
                color_found = true;
                break;
            }
        }

        if (!color_found) {
            if (color_count >= max_colors) {
                free(palette_colors);
                return NO_ERROR;
            }
            palette_colors[color_count] = pixel;
            color_count++;
        }
    }
    state->palette = palette_colors;
    state->palette_color_count = color_count;

    return NO_ERROR;
}

u32 png_palette_roundup(u32 color_count) {
    if (color_count <= 2) {
        return 2;
    }
    if (color_count <= 4) {
        return 4;
    }
    if (color_count <= 16) {
        return 16;
    }
    return 256;
}

Error png_replace_palette_colors_1b(PNG_InternalState *state) {
    return error_fatal("Palette replacement for 1-bit PNGs is not implemented yet");
}

Error png_replace_palette_colors_2b(PNG_InternalState *state) {
    return error_fatal("Palette replacement for 2-bit PNGs is not implemented yet");
}

Error png_replace_palette_colors_4b(PNG_InternalState *state) {
    return error_fatal("Palette replacement for 4-bit PNGs is not implemented yet");
}

Error png_replace_palette_colors_8b(PNG_InternalState *state) {
    const u32 pixel_size = 1 * state_channels(state);
    RGBA *palette = state->palette;
    const u32 palette_color = state->palette_color_count;

    const u32 num_pixels = state->ihdr.width * state->ihdr.height;

    u8 *new_indices = malloc(num_pixels);

    for (int i = 0; i < num_pixels; ++i) {
        const u8 *pixel = state->image_data + i * pixel_size;

        for (int color_id = 0; color_id < palette_color; ++color_id) {
            if (memcmp(pixel, &palette[color_id], pixel_size) == 0) {
                new_indices[i] = (u8) color_id;
                break;
            }
        }
    }
    free(state->image_data);
    state->image_data = new_indices;
    state->image_data_size = num_pixels;

    return NO_ERROR;
}

Error png_generate_palette(PNG_InternalState *state) {
    const u32 channel_count = state_channels(state);
    if (state->ihdr.bit_depth == 0 || state->ihdr.bit_depth > 16 || channel_count == 0) {
        return error_fatal("Unsupported PNG color type for palette generation");
    }

    Error palette_error;
    if (state->ihdr.bit_depth == 8) {
        switch (channel_count) {
            case 1: {
                palette_error = png_generate_palette_1c8b(state);
                break;
            }
            case 2: {
                palette_error = png_generate_palette_2c8b(state);
                break;
            }
            case 3: {
                palette_error = png_generate_palette_3c8b(state);
                break;
            }
            case 4: {
                palette_error = png_generate_palette_4c8b(state);
                break;
            }
            default: {
                palette_error = error_fatal("Unsupported number of channels for palette generation");
                break;
            }
        }
    } else if (state->ihdr.bit_depth == 16) {
        palette_error = error_warning_fmt("Palette generation for 16-bit PNGs is not supported");
    } else {
        palette_error = error_warning("Palette generation for PNGs with bit depth other than 8 or 16 is not supported");
    }

    RETURN_IF_FATAL_WITH_CLEANUP(palette_error, {
                                 state_free(state);
                                 });

    if (state->palette == NULL || state->palette_color_count == 0) {
        return NO_ERROR;
    }

    if (palette_error.message && !palette_error.is_fatal) {
        // state->image_data_size = state->ihdr.width * state->ihdr.height * pixel_size;
        // state->image_data = malloc(state->image_data_size);
        // memcpy(state->image_data, state->image_data, state->ihdr.width * state->ihdr.height * pixel_size);
        return NO_ERROR;
    }

    if (state->palette_color_count <= 2) {
        // 1 bit indices
        palette_error = png_replace_palette_colors_1b(state);
    } else if (state->palette_color_count <= 4) {
        // 2 bit indices
        palette_error = png_replace_palette_colors_2b(state);
    } else if (state->palette_color_count <= 16) {
        // 4 bit indices
        palette_error = png_replace_palette_colors_4b(state);
    } else {
        // 8 bit indices
        palette_error = png_replace_palette_colors_8b(state);
    }
    state->ihdr.color_type = PNG_COLOR_TYPE_INDEXED_COLOR;
    return palette_error;
}
