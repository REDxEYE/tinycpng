// Created by RED on 09.12.2025.
#pragma once
#include "mytypes.h"

int zlibng_compress_buffer(const uint8_t *data, u32 len, uint8_t **out, u32 *out_len, s32 level);

int zlibng_decompress_buffer(const uint8_t *data, u32 len, uint8_t *out, u32 out_len);
