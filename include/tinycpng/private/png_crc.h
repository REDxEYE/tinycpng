// Created by RED on 09.12.2025.
#pragma once
#include "mytypes.h"

// Taken from https://www.w3.org/TR/2003/REC-PNG-20031110/#D-CRCAppendix

/* Update a running CRC with the bytes buf[0..len-1]--the CRC
   should be initialized to all 1's, and the transmitted value
   is the 1's complement of the final running CRC (see the
   crc() routine below)). */

u32 png_update_crc(u32 crc, const u8 *buf, u32 len);

/* Return the CRC of the bytes buf[0..len-1]. */
u32 png_crc(const u8 *buf, u32 len) ;
