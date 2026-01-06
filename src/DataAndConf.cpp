#ifdef PIO_UNIT_TESTING
#include <MockEEPROM.h>
#define EEE mockEEPROM
#else
#ifdef NATIVE
#include <MockEEPROM.h>
#define EEE mockEEPROM
#else
#include <EEPROM.h>
#define EEE EEPROM
#endif
#endif
#include <Log.h>
#include "DataAndConf.h"
#include "Constants.h"

configuration::configuration()
{
  reset();
}

void configuration::reset()
{
  serial = CONF_SERIAL;
  sin_range.set(RANGE_DEFAULT_MIN, RANGE_DEFAULT_MAX); // transducer voltage divided by 4 is 667/2000mV, so the lower bound is 1/3 of the range
  cos_range.set(RANGE_DEFAULT_MIN, RANGE_DEFAULT_MAX); // transducer voltage divided by 4 is 667/2000mV, so the lower bound is 1/3 of the range
  offset = 0;
  speed_smoothing = DEFAULT_WIND_SPEED_SMOOTHING;                               // 0..100 alpha value for LPF - 100 = no smoothing
  angle_smoothing = DEFAULT_WIND_ANGLE_SMOOTHING;                               // 0..100 alpha value for LPF - 100 = no smoothing
  speed_adjustment = 100;                                                       // 0..100 speed adjustment, multiplied by 100 to have 2 decimals
  n2k_source = DEFAULT_N2K_SOURCE;                                              // default source address
  auto_cal = 0;                                                                 // auto calibration disabled by default
  calibration_score_threshold = AUTO_CALIBRATION_SCORE_THRESHOLD_DEFAULT * 100; // a calibration is valid to be committed when the score is higher than...
  usb_tracing = 1;
  vane_type = VANE_TYPE_DEFAULT;           // default vane type
  enable_stw = 0;                          // disable speed through water by default
  enable_temp = 0;                         // disable temperature by default
  stw_adjustment = 100;                    // 0..100 speed adjustment, multiplied by 100 to have 2 decimals
  stw_smoothing = DEFAULT_STW_SMOOTHING;   // 0..100 alpha value for LPF - 100 = no smoothing
  temp_smoothing = DEFAULT_TEMP_SMOOTHING; // 0..100 alpha value for LPF - 100 = no smoothing
  strncpy(ble_name, BLE_DEVICE_NAME, sizeof(ble_name) - 1);
  ble_name[sizeof(ble_name) - 1] = 0;
}
double configuration::get_stw_smoothing_factor() const
{
  return (double)stw_smoothing / 100.0;
}

double configuration::get_temp_smoothing_factor() const
{
  return (double)temp_smoothing / 100.0;
}

double configuration::get_stw_adjustement() const
{
  return (double)stw_adjustment / 100.0;
}

double configuration::get_temp_adjustment() const
{
  return (double)temp_adjustment / 100.0;
}

double configuration::get_angle_smoothing_factor() const
{
  return (double)angle_smoothing / 100.0;
}

double configuration::get_speed_smoothing_factor() const
{
  return (double)speed_smoothing / 100.0;
}

double configuration::get_calibration_threshold_factor() const
{
  return (double)calibration_score_threshold / 100.0;
}

double configuration::get_speed_adjustement() const
{
  return (double)speed_adjustment / 100.0;
}

void configuration::set_ble_name(const char *name)
{
  if (name)
    strncpy(ble_name, name, sizeof(ble_name) - 1);
  ble_name[sizeof(ble_name) - 1] = 0;
}

bool ConfPersistence::write(configuration &conf)
{
  // Log::trace("[CONF] Writing configuration to EEPROM {%d}\n", sizeof(Conf));
  if (EEE.begin(sizeof(configuration)))
  {
    Log::trace("[CONF] Writing configuration\n");
    EEE.put(0, conf);
    bool res = EEE.commit();
    Log::trace("[CONF] Calibration written {%s}\n", res ? "OK" : "KO");
    EEE.end();
    return res;
  }
  else
  {
    Log::trace("[CONF] Error writing configuration (EEPROM not initialized)\n");
    return false;
  }
}

bool ConfPersistence::read(configuration &conf)
{
  if (EEE.begin(sizeof(configuration)))
  {
    bool read = false;
    if (EEE.readChar(0) == CONF_SERIAL)
    {
      EEE.get(0, conf);
      read = true;
      Log::trace("[CAL] Read configuration\n");
      // configuration is good
    }
    EEE.end();
    if (!read)
    {
      // conf in EEPROM is not good - wipe it out
      return write(conf);
    }
    return true;
  }
  else
  {
    Log::trace("[CAL] Error initializing calibration (EEPROM not initialized)\n");
    return false;
  }
}