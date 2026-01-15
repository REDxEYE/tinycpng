// Created by RED on 10.12.2025.
#pragma once
#include "library.h"
#include "mytypes.h"
#include "png_helpers.h"

void unfilter_none(const u8 *row_data, const u8 *previous_row, u8 *output_row, u32 row_length, u32 pixel_size);

void unfilter_sub(const u8 *row_data, const u8 *previous_row, u8 *output_row, u32 row_length, u32 pixel_size);

void unfilter_up(const u8 *row_data, const u8 *previous_row, u8 *output_row, u32 row_length, u32 pixel_size);

void unfilter_average(const u8 *row_data, const u8 *previous_row, u8 *output_row, u32 row_length, u32 pixel_size);

void unfilter_paeth(const u8 *row_data, const u8 *previous_row, u8 *output_row, u32 row_length, u32 pixel_size);

typedef void (*UnfilterFn)(const u8 *row_data, const u8 *previous_row, u8 *output_row, u32 row_length, u32 pixel_size);

extern const UnfilterFn unfilters[PNG_FILTER_TYPE_COUNT];

Error unfilter_row(const PNG_InternalState *state, const u8 *row_data, u32 row_length, u8 *output_row,
                   const u8 *previous_row);

Error filter_row(const PNG_InternalState *state, const u8 *input_row, const u8 *previous_row, u8 *output_data,
                 u32 output_length, u8 **tmp_buffers);

void filter_none(const u8 *input_row, const u8 *previous_row, u8 *output_data, u32 row_length, u32 pixel_size);

void filter_sub(const u8 *input_row, const u8 *previous_row, u8 *output_data, u32 row_length, u32 pixel_size);

void filter_up(const u8 *input_row, const u8 *previous_row, u8 *output_data, u32 row_length, u32 pixel_size);

void filter_average(const u8 *input_row, const u8 *previous_row, u8 *output_data, u32 row_length, u32 pixel_size);

void filter_paeth(const u8 *input_row, const u8 *previous_row, u8 *output_data, u32 row_length, u32 pixel_size);

typedef void (*FilterFn)(const u8 *input_row, const u8 *previous_row, u8 *output_data, u32 row_length, u32 pixel_size);

extern const FilterFn filters[PNG_FILTER_TYPE_COUNT];
