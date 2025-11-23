#include <Arduino.h>
#include <Log.h>
#include <EEPROM.h>
#include "DataAndConf.h"

Conf::Conf() :
        serial(CONF_SERIAL),
        sin_range(RANGE_DEFAULT_MIN, RANGE_DEFAULT_MAX, RANGE_DEFAULT_VALID), // transducer voltage divided by 4 is 667/2000mV, so the lower bound is 1/3 of the range
        cos_range(RANGE_DEFAULT_MIN, RANGE_DEFAULT_MAX, RANGE_DEFAULT_VALID), // transducer voltage divided by 4 is 667/2000mV, so the lower bound is 1/3 of the range
        offset(0),
        speed_smoothing(0), // 0..100 alpha value for LPF - 100 = no smoothing
        angle_smoothing(0), // 0..100 alpha value for LPF - 100 = no smoothing
        speed_adjustment(100), // 0..100 speed adjustment, multiplied by 100 to have 2 decimals
        n2k_source(DEFAULT_N2K_SOURCE), // default source address
        auto_cal(0), // auto calibration disabled by default
        calibration_score_threshold(80), // a calibration is valid to be committed when the score is higher than...
        usb_tracing(1),
        vane_type(VANE_TYPE_DEFAULT)
{
  strncpy(ble_name, BLE_DEVICE_NAME, sizeof(ble_name) - 1);
}

double Conf::get_angle_smoothing_factor() const
{
  return (double)angle_smoothing / 100.0;
}

double Conf::get_speed_smoothing_factor() const
{
  return (double)speed_smoothing / 100.0;
}

double Conf::get_calibration_threshold_factor() const
{
  return (double)calibration_score_threshold / 100.0;
}

double Conf::get_speed_adjustement() const
{
  return (double)speed_adjustment / 100.0;
}

bool Conf::write()
{
  if (EEPROM.begin(sizeof(*this)))
  {
    Log::trace("[CONF] Writing configuration\n");
    EEPROM.put(0, *this);
    bool res = EEPROM.commit();
    Log::trace("[CONF] Calibration written {%s}\n", res?"OK":"KO");
    EEPROM.end();
    return res;
  }
  else
  {
      Log::trace("[CONF] Error writing configuration (EEPROM not initialized)\n");
      return false;
  }
}

bool Conf::read()
{
  if (EEPROM.begin(sizeof(*this)))
  {
    bool read = false;
    if (EEPROM.readChar(0) == CONF_SERIAL)
    {
      EEPROM.get(0, *this);
      read = true;
      Log::trace("[CAL] Read configuration\n");
      // configuration is good
    }
    EEPROM.end();
    if (!read)
    {
      // conf in EEPROM is not good - wipe it out
      return write();
    }
    return true;
  }
  else
  {
    Log::trace("[CAL] Error initializing calibration (EEPROM not initialized)\n");
    return false;
  }
}