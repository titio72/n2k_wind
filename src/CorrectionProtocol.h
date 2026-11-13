#ifndef _CORRECTION_PROTOCOL_H
#define _CORRECTION_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include "WindCorrection.h"

// Proprietary fast-packet PGN carrying the active correction table.
// Payload (little-endian):
//   0  2   manufacturer code (11 bits) | reserved (2 bits, 1s) | industry code (3 bits) = 4
//   2  1   message type = 1 (correction table)
//   3  1   payload format version = 1
//   4  4   table_id = CRC-32 over bytes 8..end
//   8  1   enabled
//   9  1   row count N
//   10 17N rows: tws u16 0.1 kn, 3 x [angle u8 deg, dA i16 0.1 deg, dS i16 0.1 kn]

#define CORRECTION_PGN 130900UL
#define CORRECTION_MSG_TYPE_TABLE 1
#define CORRECTION_PAYLOAD_VERSION 1
#define CORRECTION_MAX_PAYLOAD 223
#define CORRECTION_INDUSTRY_CODE 4

uint32_t crc32_ieee(const uint8_t *data, size_t len);

// Returns the number of bytes written, 0 if buf_len is too small.
size_t encode_correction_table(const correction_table &t, uint16_t manufacturer_code,
                               uint8_t *buf, size_t buf_len);

uint32_t correction_table_id(const correction_table &t);

// PGN 130306 byte 5: bits 0-2 reference (apparent), bit 3 corrected,
// bits 4-7 table_id low nibble when corrected.
uint8_t wind_reference_byte(bool corrected, uint32_t table_id);

#endif
