// Created by RED on 09.12.2025.

#include "png_crc.h"

/* Table of CRCs of all 8-bit messages. */
u32 crc_table[256];

/* Flag: has the table been computed? Initially false. */
u32 crc_table_computed = 0;


/* Make the table for a fast CRC. */
void make_crc_table(void) {
    for (u32 n = 0; n < 256; n++) {
        u32 c = n;
        for (u32 k = 0; k < 8; k++) {
            if (c & 1)
                c = 0xedb88320L ^ (c >> 1);
            else
                c = c >> 1;
        }
        crc_table[n] = c;
    }
    crc_table_computed = 1;
}

u32 png_update_crc(u32 crc, const u8 *buf, u32 len) {
    u32 c = crc;

    if (!crc_table_computed)
        make_crc_table();
    for (u32 n = 0; n < len; n++) {
        c = crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
    }
    return c;
}

u32 png_crc(const u8 *buf, u32 len) {
    return png_update_crc(0xffffffffL, buf, len) ^ 0xffffffffL;
}
