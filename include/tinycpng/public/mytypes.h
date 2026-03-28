// Created by RED on 09.12.2025.
#pragma once
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef float f32;
typedef double f64;

#pragma  pack(push, 1)
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} RGBA;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
}RGB;

typedef struct {
    u8 r;
    u8 g;
} RG;

#pragma pack(pop)


#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define HOST_IS_BIG_ENDIAN 1
#else
#define HOST_IS_BIG_ENDIAN 0
#endif

static inline uint16_t bswap16(uint16_t v) {
    return (uint16_t)((v >> 8) | (v << 8));
}

static inline uint32_t bswap32(uint32_t v) {
    return  (v >> 24) |
           ((v >> 8)  & 0x0000FF00u) |
           ((v << 8)  & 0x00FF0000u) |
            (v << 24);
}

static inline uint64_t bswap64(uint64_t v) {
    return  (v >> 56) |
           ((v >> 40) & 0x000000000000FF00ull) |
           ((v >> 24) & 0x0000000000FF0000ull) |
           ((v >>  8) & 0x00000000FF000000ull) |
           ((v <<  8) & 0x000000FF00000000ull) |
           ((v << 24) & 0x0000FF0000000000ull) |
           ((v << 40) & 0x00FF000000000000ull) |
            (v << 56);
}


/* host -> encoded */
static inline uint16_t host_to_le16(uint16_t v) { return HOST_IS_BIG_ENDIAN ? bswap16(v) : v; }
static inline uint32_t host_to_le32(uint32_t v) { return HOST_IS_BIG_ENDIAN ? bswap32(v) : v; }
static inline uint64_t host_to_le64(uint64_t v) { return HOST_IS_BIG_ENDIAN ? bswap64(v) : v; }

static inline uint16_t host_to_be16(uint16_t v) { return HOST_IS_BIG_ENDIAN ? v : bswap16(v); }
static inline uint32_t host_to_be32(uint32_t v) { return HOST_IS_BIG_ENDIAN ? v : bswap32(v); }
static inline uint64_t host_to_be64(uint64_t v) { return HOST_IS_BIG_ENDIAN ? v : bswap64(v); }

/* encoded -> host */
static inline uint16_t le16_to_host(uint16_t v) { return HOST_IS_BIG_ENDIAN ? bswap16(v) : v; }
static inline uint32_t le32_to_host(uint32_t v) { return HOST_IS_BIG_ENDIAN ? bswap32(v) : v; }
static inline uint64_t le64_to_host(uint64_t v) { return HOST_IS_BIG_ENDIAN ? bswap64(v) : v; }

static inline uint16_t be16_to_host(uint16_t v) { return HOST_IS_BIG_ENDIAN ? v : bswap16(v); }
static inline uint32_t be32_to_host(uint32_t v) { return HOST_IS_BIG_ENDIAN ? v : bswap32(v); }
static inline uint64_t be64_to_host(uint64_t v) { return HOST_IS_BIG_ENDIAN ? v : bswap64(v); }