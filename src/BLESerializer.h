#ifndef BLESERIALIZER_H
#define BLESERIALIZER_H

class ByteBuffer;
class Calibration;
struct wind_data;

ByteBuffer& make_message(const wind_data& wdata, const Calibration &calib, ByteBuffer& buffer);

#endif