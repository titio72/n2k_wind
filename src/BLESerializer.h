#ifndef BLESERIALIZER_H
#define BLESERIALIZER_H

class ByteBuffer;
class Calibration;
struct all_data;
struct configuration;

ByteBuffer& make_message(const configuration &conf, const all_data &wdata, const Calibration &calib, ByteBuffer& buffer);

#endif