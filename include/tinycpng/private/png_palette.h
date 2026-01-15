// Created by RED on 15.12.2025.
#pragma once
#include <string.h>

#include "error_utils.h"
#include "library.h"
#include "png_helpers.h"

Error png_generate_palette_1c8b(PNG_InternalState *state);

Error png_generate_palette_2c8b(PNG_InternalState *state);

Error png_generate_palette_3c8b(PNG_InternalState *state);

Error png_generate_palette_4c8b(PNG_InternalState *state);

u32 png_palette_roundup(u32 color_count);

Error png_replace_palette_colors_1b(PNG_InternalState *state);

Error png_replace_palette_colors_2b(PNG_InternalState *state);

Error png_replace_palette_colors_4b(PNG_InternalState *state);

Error png_replace_palette_colors_8b(PNG_InternalState *state);


Error png_generate_palette(PNG_InternalState *state);
