#include "WindSpeed.h"
#include "WindUtil.h"
#include "Utils.h"
#include "DataAndConf.h"
#include <Log.h>
/*
The vane are r=55mm from the center, so a full round is 2*pi*r
With a frequency of f HZ (there are 4 magnets, so 2 transitions per round, hence the round per seconds are f/2),
it makes a total distance of L=(f/2)*2*pi*r in 1 second, hence the speed in knots is L*3600/1852.
This is assuming that there is no friction - let's assume that the rotation speed is 50% of.
The magic number to convert Hz in Knots is 2*(pi*0.055*2600/1852) = 0.672

Raymarine says that 20Hz (10Hz considering a full revolution) is 20Knots, which would mean a factor of 1.0

ST50 Wind Speed
The ST50 instrument does not have a user-adjustable wind speed calibration factor in its software. The instrument assumes a specific rotation rate from its original masthead transducer.
Transducer Compatibility: If a newer ST60 masthead transducer (which has an egg-shaped body and spins faster) is connected to an older ST50 display (which used a cylindrical body transducer), the ST50 will over-read the wind speed.
Correction: The only intended method to correct this is to use the correct type of masthead unit for the display or to use an ST60+ display and set its calibration factor to 0.7 to match the older ST50 transducer's output.
*/

static const bool USE_SPEED_SENSOR_INTERRUPT = false; // set to true to use SpeedSensorInterrupt instead of SpeedSensor

WindSpeed::WindSpeed(int pin) : vane_type(VANE_TYPE_DEFAULT), speed_sensor(pin), speed_sensor_interrupt(pin)
{
}

WindSpeed::~WindSpeed()
{
}

void WindSpeed::read_data(wind_data &data, configuration &conf, unsigned long milliseconds)
{
  double frequency = 0.0;
  //int counter = 0;
  speed_sensor_interrupt.set_alpha(conf.get_speed_smoothing_factor());
  speed_sensor.set_alpha(conf.get_speed_smoothing_factor());

  int counter = USE_SPEED_SENSOR_INTERRUPT ? speed_sensor_interrupt.get_counter() : speed_sensor.get_counter();
  if (counter > 2 || (milliseconds - last_valid_reat_ts) > 999)
  {
    bool read = USE_SPEED_SENSOR_INTERRUPT ? speed_sensor_interrupt.read_data(milliseconds, frequency, counter) : speed_sensor.read_data(milliseconds, frequency, counter);
    if (read)
    {
      last_valid_reat_ts = milliseconds;
      data.frequency = frequency;
      data.speed = frequency * adjustment_factor * ((vane_type == VANE_TYPE_ST60) ? HZ_TO_KNOTS_ST60 : HZ_TO_KNOTS_ST50) * 0.5;
      data.speed_error = WIND_ERROR_OK;
      //Log::tracex("WIND", "SpeedSensor", "Read speed sensor: counter {%d} frequency {%.2f}Hz speed {%.2f}knots", counter, frequency, data.speed);
    }
    else
    {
      data.speed_error = WIND_ERROR_NO_SIGNAL;
    }
  }
  else
  {
    //Log::tracex("WIND", "WindSpeed", "Ignoring speed sensor reading: counter {%d} time since last valid reading {%lu}ms", counter, milliseconds - last_valid_reat_ts);
    // do nothing, keep previous value and wait for a valid reading
    // this should decrease the frequency of wind speed updates for very light winds, but reduce noise
  }
}

void WindSpeed::apply_configuration(configuration &conf)
{
  adjustment_factor = conf.get_speed_adjustement();
  vane_type = conf.vane_type;
}

// the time is in micros! called from an ISR every 1ms
void WindSpeed::loop_micros(unsigned long t)
{
  if (!USE_SPEED_SENSOR_INTERRUPT)
    speed_sensor.loop_micros(t);
}

void WindSpeed::setup()
{
  if (USE_SPEED_SENSOR_INTERRUPT)
  {
    Log::tracex("WIND", "WindSpeed", "Using SpeedSensorInterrupt for wind speed measurement");
    speed_sensor_interrupt.setup();
  }
  else
  {
    Log::tracex("WIND", "WindSpeed", "Using SpeedSensor for wind speed measurement");
    speed_sensor.setup();
  }
}