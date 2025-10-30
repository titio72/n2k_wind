#ifndef _CONF_H
#define _CONF_H
#include <Arduino.h>
#include "WindUtil.h"

#define CONF_SERIAL 12

class Conf
{
public:
  Conf(uint16_t def_l, uint16_t def_h, uint16_t validity) :
        serial(CONF_SERIAL),
        sin_range(def_l, def_h, validity), // transducer voltage divided by 4 is 667/2000mV, so the lower bound is 1/3 of the range
        cos_range(def_l, def_h, validity), // transducer voltage divided by 4 is 667/2000mV, so the lower bound is 1/3 of the range
        offset(0),
        speed_smoothing(0), // 0..255 0==no smoothing - 255 = max smoothing. Not used yet
        speed_adjustment(0.672f * 100), // factor to convert Hz to knots, multiplied by 100 to have 2 decimals
        n2k_source(32), // default source address
        auto_cal(0) // auto calibration disabled by default
  {
  }

  char serial;
  Range sin_range;
  Range cos_range;
  int16_t offset;
  uint8_t speed_smoothing;
  uint8_t angle_smoothing;
  uint8_t speed_adjustment;
  uint8_t n2k_source;
  uint8_t auto_cal;

  bool write();
  bool read();
};

#endif