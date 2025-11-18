#include <Arduino.h>
#include "WindSpeed.h"
#include "WindUtil.h"
#include "Utils.h"
#include "DataAndConf.h"

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

WindSpeed::WindSpeed(): vane_type(VANE_TYPE_DEFAULT)
{
}

WindSpeed::~WindSpeed()
{
}

void WindSpeed::read_data(wind_data &data, unsigned long milliseconds)
{
  unsigned long dt = milliseconds - last_read_time;
  last_read_time = milliseconds;

  if (dt>50) // arbitrary 50ms interval between two readings (it should be 250ms)
  {
    double alpha = data.conf.get_speed_smoothing_factor();
    smooth_counter = (double)counter * alpha + smooth_counter * (1.0 - alpha);
    data.frequency = (dt>50) ? (smooth_counter * 1000.0 / (double)dt)  : 0.0;
    data.frequency /= 2.0; // divide by 2 because there are two sensors, hence two squares per revolution
    data.speed = data.frequency * adjustment_factor * ((vane_type==VANE_TYPE_ST60)?HZ_TO_KNOTS_ST60:HZ_TO_KNOTS_ST50);
    data.error_speed = WIND_ERROR_OK;
    counter = 0;
  }

}

void WindSpeed::set_speed_adjustment(double f)
{
  adjustment_factor = f;
}

void WindSpeed::apply_configuration(Conf& conf)
{
  set_speed_adjustment(conf.get_speed_adjustement());
  vane_type = conf.vane_type;
}

// the time is in micros! called from an ISR every 1ms
void IRAM_ATTR WindSpeed::loop_micros(unsigned long t)
{
  int new_state = digitalRead(SPEED_PIN);
  if (new_state!=state)
  {
    counter++;
    state = new_state;
  }
}

void WindSpeed::setup()
{
  pinMode(SPEED_PIN, INPUT);
}