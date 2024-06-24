#ifndef _WIND_DIRECTION_H
#define _WIND_DIRECTION_H
#include <stdint.h>
#include "SinCosDecoder.h"
#include "WindAngleSImulator.h"

class WindDirection
{
public:
    WindDirection(SinCosDecoder &wcalc, bool simulate);
    ~WindDirection();

    void get_sincos(uint16_t &sin, uint16_t &cos);
    double get_expected();

    void get_angle(uint16_t &i_sin, uint16_t &i_cos, double &angle, double &err, int &error);

    void setup();

    void loop_micros(uint64_t now_micros);

private:
    WindAngleSimulator simulator;
    SinCosDecoder &w_calc;
    bool simulate;
    double expected;
    uint16_t *sinBuffer, *cosBuffer;
    uint8_t ix_buffer_sin, ix_buffer_cos;
    uint32_t sumSin, sumCos;
};




#endif