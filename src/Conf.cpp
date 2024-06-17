#include <Arduino.h>
#include <Log.h>
#include <EEPROM.h>
#include "Conf.h"

bool Conf::write()
{
  if (EEPROM.begin(sizeof(*this)))
  {
    Log::trace("[CONF] Writing calibration: sin {%d %d} cos {%d %d}\n",
               sin_range.low(), sin_range.high(), cos_range.low(), cos_range.high());
    EEPROM.put(0, *this);
    if (!EEPROM.commit())
    {
      Log::trace("[CONF] Error writing calibration\n");
    }
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
      Log::trace("[CAL] Read calibration: sin {%d %d} cos {%d %d}\n", sin_range.low(), sin_range.high(), cos_range.low(), cos_range.high());
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