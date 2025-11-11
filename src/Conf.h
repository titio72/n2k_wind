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
        auto_cal(0), // auto calibration disabled by default
        calibration_score_threshold(80), // a calibration is valid to be committed when the score is higher than...
        usb_tracing(1)
  {
  }

  double get_angle_smoothing_factor()
  {
    return (double)angle_smoothing / 50.0;
  }

  double get_speed_smoothing_factor()
  {
    return (double)speed_smoothing / 50.0;
  }

  double get_calibration_threshold_factor()
  {
    return (double)calibration_score_threshold / 100.0;
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
  uint8_t calibration_score_threshold;
  uint8_t usb_tracing;

  bool write();
  bool read();
};

#endif