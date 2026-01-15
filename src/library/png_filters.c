// Created by RED on 10.12.2025.

#include "png_filters.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

u8 paeth_predictor(u8 a, u8 b, u8 c) {
    const s32 aa = a;
    const s32 bb = b;
    const s32 cc = c;
    const s32 p = aa + bb - cc;
    const s32 pa = abs(p - aa);
    const s32 pb = abs(p - bb);
    const s32 pc = abs(p - cc);

    if (pa <= pb && pa <= pc) {
        return a;
    }
    if (pb <= pc) {
        return b;
    }
    return c;
}


void unfilter_none(const u8 *row_data, const u8 *previous_row, u8 *output_row, const u32 row_length, u32 pixel_size) {
    memcpy(output_row, row_data, row_length);
}

void unfilter_sub(const u8 *row_data, const u8 *previous_row, u8 *output_row, u32 row_length, u32 pixel_size) {
    memcpy(output_row, row_data, pixel_size);
    for (u32 i = pixel_size; i < row_length; i++) {
        const u8 x = row_data[i];
        const u8 a = output_row[i - pixel_size];
        output_row[i] = x + a;
    }
}

void unfilter_up(const u8 *row_data, const u8 *previous_row, u8 *output_row, u32 row_length, u32 pixel_size) {
    for (u32 i = 0; i < row_length; i++) {
        const u8 x = row_data[i];
        const u8 b = previous_row ? previous_row[i] : 0;
        output_row[i] = x + b;
    }
}

void unfilter_average(const u8 *row_data, const u8 *previous_row, u8 *output_row, u32 row_length, u32 pixel_size) {
    for (u32 i = 0; i < row_length; i++) {
        const u8 x = row_data[i];
        const u8 a = i < pixel_size ? 0 : output_row[i - pixel_size];
        const u8 b = previous_row ? previous_row[i] : 0;
        output_row[i] = x + (((u16) a + (u16) b) / 2);
    }
}

void unfilter_paeth(const u8 *row_data, const u8 *previous_row, u8 *output_row, u32 row_length, u32 pixel_size) {
    for (u32 i = 0; i < row_length; i++) {
        const u8 x = row_data[i];
        const u8 a = i < pixel_size ? 0 : output_row[i - pixel_size];
        const u8 b = previous_row ? previous_row[i] : 0;
        const u8 c = (previous_row && i >= pixel_size) ? previous_row[i - pixel_size] : 0;
        output_row[i] = x + paeth_predictor(a, b, c);
    }
}

const UnfilterFn unfilters[] = {
    unfilter_none,
    unfilter_sub,
    unfilter_up,
    unfilter_average,
    unfilter_paeth
};


// TODO: Copy row data(without type byte) to output and process in-place
Error unfilter_row(const PNG_InternalState *state, const u8 *row_data, const u32 row_length, u8 *output_row,
                   const u8 *previous_row) {
    u32 expected_row_size = state_bytes_per_row(state);
    if (row_length != 1 + expected_row_size) {
        return error_fatal("Row length does not match expected size for unfiltering");
    }
    const PNGFilterType filter_type = (PNGFilterType) row_data[0];
    if (filter_type >= PNG_FILTER_TYPE_COUNT) {
        return error_fatal_fmt("Invalid PNG filter type %u", filter_type);
    }

    u32 pixel_size = PNG_MAX(1, state->ihdr.bit_depth / 8 * state_channels(state));

    unfilters[filter_type](row_data + 1, previous_row, output_row, row_length - 1, pixel_size);
    return NO_ERROR;
}

void filter_none(const u8 *input_row, const u8 *previous_row, u8 *output_data, u32 output_length, u32 pixel_size) {
    memcpy(output_data, input_row, output_length);
}

void filter_sub(const u8 *input_row, const u8 *previous_row, u8 *output_data, u32 output_length, u32 pixel_size) {
    memcpy(output_data, input_row, pixel_size);
    for (u32 i = pixel_size; i < output_length; i++) {
        const u8 x = input_row[i];
        const u8 a = input_row[i - pixel_size];
        output_data[i] = x - a;
    }
}

void filter_up(const u8 *input_row, const u8 *previous_row, u8 *output_data, u32 output_length, u32 pixel_size) {
    for (u32 i = 0; i < output_length; i++) {
        const u8 x = input_row[i];
        const u8 b = previous_row ? previous_row[i] : 0;
        output_data[i] = x - b;
    }
}

void filter_average(const u8 *input_row, const u8 *previous_row, u8 *output_data, u32 output_length, u32 pixel_size) {
    for (u32 i = 0; i < output_length; i++) {
        const u8 x = input_row[i];
        const u8 a = i < pixel_size ? 0 : input_row[i - pixel_size];
        const u8 b = previous_row ? previous_row[i] : 0;
        output_data[i] = x - (((u16) a + (u16) b) / 2);
    }
}

void filter_paeth(const u8 *input_row, const u8 *previous_row, u8 *output_data, u32 output_length, u32 pixel_size) {
    for (u32 i = 0; i < output_length; i++) {
        const u8 x = input_row[i];
        const u8 a = i < pixel_size ? 0 : input_row[i - pixel_size];
        const u8 b = previous_row ? previous_row[i] : 0;
        const u8 c = (previous_row && i >= pixel_size) ? previous_row[i - pixel_size] : 0;
        output_data[i] = x - paeth_predictor(a, b, c);
    }
}

const FilterFn filters[] = {
    filter_none,
    filter_sub,
    filter_up,
    filter_average,
    filter_paeth
};

typedef struct {
    uint32_t sad;
    uint32_t zero_count;
    uint32_t run_score;
    float entropy;
    int64_t total_score;
} Metrics;

#define METRIC_ZERO_BONUS      3
#define METRIC_RUN_BONUS       3
#define METRIC_ENTROPY_BONUS      1024

uint32_t u8_abs_s8(uint8_t b) {
    int8_t s = (int8_t) b;
    return (uint32_t) (s < 0 ? -s : s);
}

void compute_metrics(const uint8_t *current_row,
                     uint32_t row_length,
                     Metrics *out_metrics
) {
    uint32_t freq[256] = {0};
    uint32_t run_score = 0;
    uint64_t sad = 0;

    if (row_length == 0) {
        *out_metrics = (Metrics){0};
        return;
    }

    uint8_t prev_byte = current_row[0];
    freq[prev_byte]++;
    sad += u8_abs_s8(prev_byte);

    uint32_t run_len = 1;

    for (uint32_t i = 1; i < row_length; ++i) {
        const uint8_t b = current_row[i];

        freq[b]++;
        sad += u8_abs_s8(b);

        if (b == prev_byte) {
            ++run_len;
        } else {
            if (run_len >= 2) run_score += run_len - 1;
            run_len = 1;
            prev_byte = b;
        }
    }
    if (run_len >= 2) run_score += run_len - 1;

    float entropy = 0;
    for (uint32_t i = 0; i < 256; ++i) {
        if (freq[i] > 0) {
           float p = (float) freq[i] / (float) row_length;
           if (p>0) {
                entropy = entropy  - p *log2f(p);
           }
        }
    }

    int64_t score = (int64_t) sad;
    score -= (int64_t) freq[0] * METRIC_ZERO_BONUS;
    score -= (int64_t) run_score * METRIC_RUN_BONUS;
    score -= (int64_t) ((1.f/entropy) * METRIC_ENTROPY_BONUS);

    out_metrics->sad = (uint32_t) sad;
    out_metrics->zero_count = freq[0];
    out_metrics->run_score = run_score;
    out_metrics->entropy = entropy;
    out_metrics->total_score = score;
}

PNGFilterType choose_filter_for_row(const PNG_InternalState *state,
                                    const uint8_t *input_row,
                                    const uint8_t *prev_row,
                                    uint32_t row_bytes, u8 **tmp) {
    uint32_t pixel_size = state_pixel_size(state);

    Metrics m[PNG_FILTER_TYPE_COUNT];

    for (uint32_t f = 0; f < PNG_FILTER_TYPE_COUNT; ++f) {
        filters[f](input_row, prev_row, tmp[f], row_bytes, pixel_size);
        compute_metrics(tmp[f], row_bytes, &m[f]);
    }

    uint32_t best = 0;
    int64_t best_score = m[0].total_score;
    for (uint32_t f = 1; f < PNG_FILTER_TYPE_COUNT; ++f) {
        if (m[f].total_score < best_score) {
            best_score = m[f].total_score;
            best = f;
        }
    }

    // printf("Filter metrics:\n");
    // for (uint32_t f = 0; f < PNG_FILTER_TYPE_COUNT; ++f) {
    //     printf("  Filter %u: SAD=%u, ZeroCount=%u, RunScore=%u, entropy=%.4f(%.5f), TotalScore=%lld%s\n",
    //            f, m[f].sad, m[f].zero_count, m[f].run_score, (1.f/m[f].entropy), (1.f/m[f].entropy) * METRIC_ENTROPY_BONUS,
    //            m[f].total_score, (f == best) ? " <-- Best" : "");
    // }
    return (PNGFilterType) best;
}

Error filter_row(const PNG_InternalState *state, const u8 *input_row, const u8 *previous_row, u8 *output_data,
                 const u32 output_length, u8 **tmp_buffers) {

    if (state->palette!=NULL) {
        filter_none(input_row, previous_row, output_data + 1, output_length - 1, state_pixel_size(state));
        output_data[0] = PNG_FILTER_TYPE_NONE;
        return NO_ERROR;
    }
    const PNGFilterType best_filter = choose_filter_for_row(state, input_row, previous_row, output_length - 1,
                                                            tmp_buffers);
    output_data[0] = (uint8_t) best_filter;
    memcpy(output_data + 1, tmp_buffers[best_filter], output_length-1);
    return NO_ERROR;
}
