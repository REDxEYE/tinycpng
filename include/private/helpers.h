// Created by RED on 11.12.2025.
#pragma once
#include "mytypes.h"


#define FROM_BE16(u8_ptr) (((u16)(u8_ptr)[0] << 8) | ((u16)(u8_ptr)[1]))
#define FROM_BE32(u8_ptr) (((u32)(u8_ptr)[0] << 24) | ((u32)(u8_ptr)[1] << 16) | \
                           ((u32)(u8_ptr)[2] << 8)  | ((u32)(u8_ptr)[3]))
#define FROM_BE64(u8_ptr) (((u64)(u8_ptr)[0] << 56) | ((u64)(u8_ptr)[1] << 48) | \
                           ((u64)(u8_ptr)[2] << 40) | ((u64)(u8_ptr)[3] << 32) | \
                           ((u64)(u8_ptr)[4] << 24) | ((u64)(u8_ptr)[5] << 16) | \
                           ((u64)(u8_ptr)[6] << 8)  | ((u64)(u8_ptr)[7]))


u32 str_len_safe(const char *str, u32 max_len);
