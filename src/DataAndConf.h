#ifndef _CONF_H
#define _CONF_H
#include <Arduino.h>
#include "WindUtil.h"

#define CONF_SERIAL 12

class Conf
{
public:
  Conf(uint16_t def_l = RANGE_DEFAULT_MIN, uint16_t def_h = RANGE_DEFAULT_MAX, uint16_t validity = RANGE_DEFAULT_VALID) :
        serial(CONF_SERIAL),
        sin_range(def_l, def_h, validity), // transducer voltage divided by 4 is 667/2000mV, so the lower bound is 1/3 of the range
        cos_range(def_l, def_h, validity), // transducer voltage divided by 4 is 667/2000mV, so the lower bound is 1/3 of the range
        offset(0),
        speed_smoothing(0), // 0..100 alpha value for LPF - 100 = no smoothing
        angle_smoothing(0), // 0..100 alpha value for LPF - 100 = no smoothing
        speed_adjustment(100), // 0..100 speed adjustment, multiplied by 100 to have 2 decimals
        n2k_source(DEFAULT_WIND_N2K_SOURCE), // default source address
        auto_cal(0), // auto calibration disabled by default
        calibration_score_threshold(80), // a calibration is valid to be committed when the score is higher than...
        usb_tracing(1),
        vane_type(VANE_TYPE_DEFAULT)
  {
  }

  double get_angle_smoothing_factor()
  {
    return (double)angle_smoothing / 100.0;
  }

  double get_speed_smoothing_factor()
  {
    return (double)speed_smoothing / 100.0;
  }

  double get_calibration_threshold_factor()
  {
    return (double)calibration_score_threshold / 100.0;
  }

  double get_speed_adjustement()
  {
    return (double)speed_adjustment / 100.0;
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
  uint8_t vane_type;

  bool write();
  bool read();
};

struct wind_data
{
    // angle data
    double angle = 0.0;
    double smooth_angle = 0.0;
    double ellipse = 1.0;
    int error = WIND_ERROR_NO_CAL_OR_SIGNAL;
    uint16_t i_sin = 0;
    uint16_t i_cos = 0;

    // speed data
    double speed = 0.0;
    double frequency = 0.0;
    int error_speed = WIND_ERROR_NO_CAL_OR_SIGNAL;

    // system data
    uint8_t n2k_err = 1;
    unsigned long heap = 0;
    
    // configuration
    //uint16_t offset = 0;
    //double angle_smoothing_factor = 1.0;
    //double speed_smoothing_factor = 1.0;
    //double calibration_score_threshold = 0.8;

    Conf conf;

    double get_out_angle()
    {
      return norm_deg(smooth_angle + conf.offset);
    }
};
#endif