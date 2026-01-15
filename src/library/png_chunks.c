// Created by RED on 11.12.2025.

#include "png_chunks.h"

#include <stdlib.h>
#include <string.h>

#include "helpers.h"

Error IHDRChunk_from_bytes(IHDRChunk *chunk, const u8 *data) {
    chunk->width = FROM_BE32(data);
    chunk->height = FROM_BE32(data + 4);
    chunk->bit_depth = data[8];
    chunk->color_type = (PNGColorType) data[9];
    chunk->compression_method = data[10];
    chunk->filter_method = data[11];
    chunk->interlace_method = data[12];

    return NO_ERROR;
}

Error IHDRChunk_to_bytes(const IHDRChunk *chunk, u8 *buffer, u32 buffer_size) {
    if (buffer_size<13) {
        return error_fatal("Buffer too small for IHDR chunk serialization");
    }
    buffer[0] = (u8) ((chunk->width >> 24) & 0xFF);
    buffer[1] = (u8) ((chunk->width >> 16) & 0xFF);
    buffer[2] = (u8) ((chunk->width >> 8) & 0xFF);
    buffer[3] = (u8) ((chunk->width) & 0xFF);
    buffer[4] = (u8) ((chunk->height >> 24) & 0xFF);
    buffer[5] = (u8) ((chunk->height >> 16) & 0xFF);
    buffer[6] = (u8) ((chunk->height >> 8) & 0xFF);
    buffer[7] = (u8) ((chunk->height) & 0xFF);
    buffer[8] = chunk->bit_depth;
    buffer[9] = (u8) chunk->color_type;
    buffer[10] = chunk->compression_method;
    buffer[11] = chunk->filter_method;
    buffer[12] = chunk->interlace_method;
    return NO_ERROR;
}

Error sRGBChunk_from_bytes(sRGBChunk *chunk, const u8 *data) {
    chunk->rendering_intent = (PNGRenderingIntent) data[0];

    return NO_ERROR;
}

Error gAMAChunk_from_bytes(gAMAChunk *chunk, const u8 *data) {
    chunk->gamma_fixed_point = FROM_BE32(data);

    return NO_ERROR;
}

Error cHRMChunk_from_bytes(cHRMChunk *chunk, const u8 *data) {
    chunk->white_point_x = FROM_BE32(data);
    chunk->white_point_y = FROM_BE32(data + 4);
    chunk->red_x = FROM_BE32(data + 8);
    chunk->red_y = FROM_BE32(data + 12);
    chunk->green_x = FROM_BE32(data + 16);
    chunk->green_y = FROM_BE32(data + 20);
    chunk->blue_x = FROM_BE32(data + 24);
    chunk->blue_y = FROM_BE32(data + 28);

    return NO_ERROR;
}

Error pHYsChunk_from_bytes(pHYsChunk *chunk, const u8 *data) {
    chunk->pixels_per_unit_x = FROM_BE32(data);
    chunk->pixels_per_unit_y = FROM_BE32(data + 4);
    chunk->unit_specifier = (PNGUnitSpecifier) data[8];

    return NO_ERROR;
}

Error tEXtChunk_from_bytes(tEXtChunk *chunk, const u8 *data, u32 length) {
    u32 keyword_length = str_len_safe((const char *) data, length);
    if (keyword_length == length) {
        return error_fatal("Invalid tEXt chunk: missing null separator");
    }
    chunk->keyword = (const char *) malloc(keyword_length + 1);
    if (!chunk->keyword) {
        return error_fatal("Failed to allocate memory for tEXt chunk keyword");
    }
    memcpy((void *) chunk->keyword, data, keyword_length);
    ((char *) chunk->keyword)[keyword_length] = '\0';
    const u32 text_length = length - (keyword_length + 1);
    chunk->text = (const char *) malloc(text_length + 1);
    if (!chunk->text) {
        free((void *) chunk->keyword);
        return error_fatal("Failed to allocate memory for tEXt chunk text");
    }
    memcpy((void *) chunk->text, data + keyword_length + 1, text_length);
    ((char *) chunk->text)[text_length] = '\0';

    return NO_ERROR;
}

void tExtChunk_free(tEXtChunk *chunk) {
    if (chunk->keyword) {
        free((void *) chunk->keyword);
        chunk->keyword = NULL;
    }
    if (chunk->text) {
        free((void *) chunk->text);
        chunk->text = NULL;
    }
}

Error iTXtChunk_from_bytes(iTXtChunk *chunk, const u8 *data, u32 length) {
    u32 offset = 0;
    u32 keyword_length = str_len_safe((const char *) data, length);
    if (keyword_length == length) {
        return error_fatal("Invalid iTXt chunk: missing null separator after keyword");
    }
    chunk->keyword = (const char *) malloc(keyword_length + 1);
    if (!chunk->keyword) {
        return error_fatal("Failed to allocate memory for iTXt chunk keyword");
    }
    memcpy((void *) chunk->keyword, data, keyword_length);
    ((char *) chunk->keyword)[keyword_length] = '\0';
    offset += keyword_length + 1;

    if (offset + 2 > length) {
        free((void *) chunk->keyword);
        return error_fatal("Invalid iTXt chunk: missing compression flag and method");
    }
    chunk->compression_flag = data[offset];
    chunk->compression_method = data[offset + 1];
    offset += 2;

    u32 language_tag_length = str_len_safe((const char *) (data + offset), length - offset);
    if (language_tag_length == length - offset) {
        free((void *) chunk->keyword);
        return error_fatal("Invalid iTXt chunk: missing null separator after language tag");
    }
    chunk->language_tag = (u8 *) malloc(language_tag_length + 1);
    if (!chunk->language_tag) {
        free((void *) chunk->keyword);
        return error_fatal("Failed to allocate memory for iTXt chunk language tag");
    }
    memcpy(chunk->language_tag, data + offset, language_tag_length);
    chunk->language_tag[language_tag_length] = '\0';
    offset += language_tag_length + 1;

    u32 translated_keyword_length = str_len_safe((const char *) (data + offset), length - offset);
    if (translated_keyword_length == length - offset) {
        free((void *) chunk->keyword);
        free(chunk->language_tag);
        return error_fatal("Invalid iTXt chunk: missing null separator after translated keyword");
    }
    chunk->translated_keyword = (u8 *) malloc(translated_keyword_length + 1);
    if (!chunk->translated_keyword) {
        free((void *) chunk->keyword);
        free(chunk->language_tag);
        return error_fatal("Failed to allocate memory for iTXt chunk translated keyword");
    }
    memcpy(chunk->translated_keyword, data + offset, translated_keyword_length);
    chunk->translated_keyword[translated_keyword_length] = '\0';
    offset += translated_keyword_length + 1;
    const u32 text_length = length - offset;
    chunk->text = (const char *) malloc(text_length + 1);
    if (!chunk->text) {
        free((void *) chunk->keyword);
        free(chunk->language_tag);
        free(chunk->translated_keyword);
        return error_fatal("Failed to allocate memory for iTXt chunk text");
    }
    memcpy((void *) chunk->text, data + offset, text_length);
    ((char *) chunk->text)[text_length] = '\0';

    return NO_ERROR;
}

void iTxtChunk_free(iTXtChunk *chunk) {
    if (chunk->keyword) {
        free((void *) chunk->keyword);
        chunk->keyword = NULL;
    }
    if (chunk->language_tag!=NULL) {
        free(chunk->language_tag);
        chunk->language_tag = NULL;
    }
    if (chunk->translated_keyword!=NULL) {
        free(chunk->translated_keyword);
        chunk->translated_keyword = NULL;
    }
    if (chunk->text!=NULL) {
        free((void *) chunk->text);
        chunk->text = NULL;
    }
}

Error iCCPChunk_from_bytes(iCCPChunk *chunk, const u8 *data, u32 length) {
    u32 offset = 0;
    u32 profile_name_length = str_len_safe((const char *) data, length);
    if (profile_name_length == length) {
        return error_fatal("Invalid iCCP chunk: missing null separator after profile name");
    }
    chunk->profile_name = (const char *) malloc(profile_name_length + 1);
    if (!chunk->profile_name) {
        return error_fatal("Failed to allocate memory for iCCP chunk profile name");
    }
    memcpy((void *) chunk->profile_name, data, profile_name_length);
    ((char *) chunk->profile_name)[profile_name_length] = '\0';
    offset += profile_name_length + 1;

    if (offset + 1 > length) {
        free((void *) chunk->profile_name);
        return error_fatal("Invalid iCCP chunk: missing compression method");
    }
    chunk->compression_method = data[offset];
    offset += 1;

    const u32 compressed_profile_size = length - offset;
    chunk->compressed_profile_data = (u8 *) malloc(compressed_profile_size);
    if (!chunk->compressed_profile_data) {
        free((void *) chunk->profile_name);
        return error_fatal("Failed to allocate memory for iCCP chunk compressed profile data");
    }
    memcpy(chunk->compressed_profile_data, data + offset, compressed_profile_size);
    chunk->compressed_profile_size = compressed_profile_size;

    return NO_ERROR;
}

void iCCPChunk_free(iCCPChunk *chunk) {
    if (chunk->profile_name) {
        free((void *) chunk->profile_name);
        chunk->profile_name = NULL;
    }
    if (chunk->compressed_profile_data) {
        free(chunk->compressed_profile_data);
        chunk->compressed_profile_data = NULL;
    }
}
