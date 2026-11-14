#ifndef BLESERIALIZER_H
#define BLESERIALIZER_H

#include <stddef.h>
#include <stdint.h>

class ByteBuffer;
struct correction_table;
class Calibration;
struct all_data;
struct configuration;

ByteBuffer& make_message(const configuration &conf, const all_data &wdata, const Calibration &calib, ByteBuffer& buffer);

// Correction table characteristic: the PGN 130900 payload, byte for byte.
// Returns the length (10..180), or 0 if buf_len is too small.
size_t make_table_message(const correction_table &t, uint8_t *buf, size_t buf_len);

#endif