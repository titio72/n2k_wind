#ifndef _CONF_H
#define _CONF_H
#include <Arduino.h>
#include "WindUtil.h"

#define CONF_SERIAL 12

class Conf
{
public:
  Conf();

  double get_angle_smoothing_factor() const;

  double get_speed_smoothing_factor() const;

  double get_calibration_threshold_factor() const;

  double get_speed_adjustement() const;

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
  char ble_name[16];

  bool write();
  bool read();
};

struct wind_data
{
    // angle data
    double angle = 0.0;
    double smooth_angle = 0.0;
    double ellipse = 1.0;
    uint8_t angle_error = WIND_ERROR_NO_SIGNAL;
    uint16_t i_sin = 0;
    uint16_t i_cos = 0;

    // speed data
    double speed = 0.0;
    double frequency = 0.0;
    uint8_t speed_error = WIND_ERROR_NO_SIGNAL;

    // system data
    uint8_t n2k_err = 1;
    unsigned long heap = 0;

    Conf conf;

    double get_out_angle() const
    {
      return norm_deg(smooth_angle + conf.offset);
    }
};
#endif