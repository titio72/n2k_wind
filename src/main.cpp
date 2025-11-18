#include <Arduino.h>
#include <math.h>
#include <Log.h>

#include "SinCosDecoder.h"
#include "Wind360.h"
#include "WindSpeed.h"
#include "WindDirection.h"
#include "LedDriver.h"
#include "WindUtil.h"
#include "DataAndConf.h"
#include "AutoCalibration.h"
#include "CommandHandler.h"
#include "BLEWind.hpp"
#include "N2kWind.hpp"
#include "WindSystem.h"

void on_n2k_source(unsigned char old_src, unsigned char new_src);
bool on_calibration_complete(Range &s_range, Range &c_range);

#pragma region Global objects
Conf conf;
WindDirection wind_direction;
WindSpeed wind_speed;
AutoCalibration auto_calibration(on_calibration_complete);
LedDriver led;
CommandHandler cmd_handler(conf, auto_calibration);
BLEWind ble_wind(cmd_handler);
N2KWind n2k_wind(on_n2k_source);
WindSystem &wind_sys(WindSystem::get_instance());
#pragma endregion

// ESP32 hw timer callback
inline void on_timer(unsigned long t_micros)
{
  wind_speed.loop_micros(t_micros);
  wind_direction.loop_micros(t_micros);
}

void on_n2k_source(unsigned char old_src, unsigned char new_src)
{
  Log::trace("[N2K] Source changed from {%d} to {%d}\n", old_src, new_src);
  conf.n2k_source = new_src;
  conf.write();
}

void setup()
{
  Serial.begin(115200);
  msleep(1000);

  // read configuration from eeprom
  conf.read();
  wind_sys.enable_usb_tracing(conf.usb_tracing);

  // initialize n2k & ble
  n2k_wind.setup();
  ble_wind.setup();

  // initialize wind measurement
  wind_speed.setup();
  wind_direction.setup();

  // initialize leds
  led.setup();

  // init system (CPU freq & timers)
  wind_sys.set_timer_callback(on_timer);
  wind_sys.setup();

  Log::trace("[APP] Setup done\n");
}

Wind360 &get_calibration_progress()
{
  return auto_calibration.get_wind360();
}

void update_led(const wind_data &wdata)
{
  led.set_blue(ble_wind.is_alive());
  led.set_error(wdata.error);
  led.set_calibration(wdata.conf.auto_cal);
}

void do_log(const wind_data &wdata)
{
  Log::trace("[APP] Wind %s Sin/Cos {%d[%d..%d]/%d[%d..%d] %.1f} Dir {%5.1f[%5.1f]} Speed {%.1fKn/%.1fHz} Auto {%d}",
             wdata.conf.vane_type ? "ST60" : "ST50",
             wdata.i_sin, conf.sin_range.low(), conf.sin_range.high(),
             wdata.i_cos, conf.cos_range.low(), conf.cos_range.high(),
             wdata.ellipse, wdata.angle, wdata.smooth_angle,
             wdata.speed, wdata.frequency,
             auto_calibration.is_enabled());

  Wind360 &cal_progr = get_calibration_progress();
  if (cal_progr.size())
  {
    Log::trace(" {%02d/%02d/%02d} {", cal_progr.progress(), cal_progr.size(), cal_progr.buffer_size());
    for (int i = 0; i < cal_progr.buffer_size(); i++)
      Log::trace(" %02x", cal_progr.get_data(i));
    Log::trace(" - %.2f}", cal_progr.get_score());
  }
  Log::trace("                \r\n");
}

bool on_calibration_complete(Range &s_range, Range &c_range)
{
  if (s_range.is_valid() && c_range.is_valid())
  {
    conf.sin_range.set(s_range);
    conf.cos_range.set(c_range);
    if (conf.write())
    {
      Log::trace("[CAL] Calibration updated : sin {%d %d} cos {%d %d}\n",
               conf.sin_range.low(), conf.sin_range.high(),
               conf.cos_range.low(), conf.cos_range.high());
               return true;
    }
  }
  Log::trace("[CAL] Calibration invalid : sin {%d %d} cos {%d %d}\n",
              conf.sin_range.low(), conf.sin_range.high(),
              conf.cos_range.low(), conf.cos_range.high());
  return false;
}

void loop()
{
  static wind_data wdata;
  unsigned long t = micros();
  static unsigned long t0 = t;
  static unsigned long n2k_t0 = t;
  if (check_elapsed(t, t0, MAIN_LOOP_PERIOD_LOW_FREQ))
  {
    wdata.heap = get_free_mem();
    unsigned long t_ms = t / 1000L;

    // reload configuration
    wdata.conf = conf;
    wind_speed.apply_configuration(conf);
    wind_direction.apply_configuration(conf);
    auto_calibration.apply_configuration(conf);

    // read wind
    wind_direction.read_data(wdata, t_ms);
    wind_speed.read_data(wdata, t_ms);

    // manage calibration
    if (t_ms > CALIBRATION_SAMPLING_EXCLUSION_PERIOD) // do not sample for X seconds after restart
    {
      auto_calibration.record_reading(wdata.i_sin, wdata.i_cos, wdata.angle);
    }

    // update led
    update_led(wdata);
    led.loop(t_ms);

    // send data to bluetooth
    ble_wind.send_BLE(wdata, auto_calibration);
    ble_wind.loop(t_ms);

    // send data to n2k
    if (check_elapsed(t, n2k_t0, WIND_N2K_DATA_FREQ))
    {
      n2k_wind.send_N2K(wdata.get_out_angle(), wdata.speed);
    }
    n2k_wind.loop(t_ms);

    wdata.n2k_err = n2k_wind.is_n2k_err() ? 1 : 0;

    do_log(wdata);
  }
}
