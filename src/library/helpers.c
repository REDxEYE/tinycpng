// Created by RED on 16.12.2025.
#include "helpers.h"

u32 str_len_safe(const char *str, u32 max_len) {
    u32 len = 0;
    while (len < max_len && str[len] != '\0') {
        len++;
    }
    return len;
}
