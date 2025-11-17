#include <Arduino.h>
#include <Log.h>
#include <EEPROM.h>
#include "DataAndConf.h"

bool Conf::write()
{
  if (EEPROM.begin(sizeof(*this)))
  {
    Log::trace("[CONF] Writing calibration: sin {%d %d} cos {%d %d} offset {%d} speed_adj {%d}\n",
               sin_range.low(), sin_range.high(), cos_range.low(), cos_range.high(), offset, speed_adjustment);
    EEPROM.put(0, *this);
    bool res = EEPROM.commit();
    Log::trace("[CONF] Calibration written {%d}\n", res);
    EEPROM.end();
    return true;
  }
  else
  {
      Log::trace("[CONF] Error writing calibration (EEPROM not initialized)\n");
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
      Log::trace("[CAL] Read calibration: sin {%d %d} cos {%d %d} offset {%d} speed_adj {%d} angle_smoothing {%d} speed_smoothing {%d}\n", 
        sin_range.low(), sin_range.high(), cos_range.low(), cos_range.high(), offset, speed_adjustment, angle_smoothing, speed_smoothing);
      // configuration is good
    }
    EEPROM.end();
    if (!read)
    {
      // conf in EEPROM is not good - wipe it out
      write();
    }
    return true;
  }
  else
  {
    Log::trace("[CAL] Error initializing calibration (EEPROM not initialized)\n");
    return false;
  }
}