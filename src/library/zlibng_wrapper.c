// Created by RED on 09.12.2025.
#include "zlibng_wrapper.h"

#include <stdlib.h>

#include "zlib-ng.h"

int zlibng_compress_buffer(const u8 *data, const u32 len, u8 **out, u32 *out_len, const s32 level) {
    const u32 bound = zng_compressBound(len);
    u8 *dst = (u8 *) malloc(bound);
    if (!dst) return Z_MEM_ERROR;

    size_t size = bound;
    const s32 ret = zng_compress2(dst, &size, data, len, level);
    if (ret != Z_OK) {
        free(dst);
        return ret;
    }

    *out = dst;
    *out_len = (u32) size;
    return Z_OK;
}

int zlibng_decompress_buffer(const u8 *data, const u32 len, u8 *out, const u32 out_len) {
    size_t dst_len = out_len;
    return zng_uncompress(out, &dst_len, data, len);
}
