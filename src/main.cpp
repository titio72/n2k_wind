#ifndef NATIVE
#include <Arduino.h>
#include <math.h>
#include <Log.h>

#include "Wind360.h"
#include "WindSpeed.h"
#include "WindDirection.h"
#include "LedDriver.h"
#include "DataAndConf.h"
#include "Calibration.h"
#include "CommandHandler.h"
#include "BLEWind.h"
#include "N2kWind.hpp"
#include "WindSystem.h"

void on_n2k_source(unsigned char old_src, unsigned char new_src);
bool on_calibration_complete(const Range &s_range, const Range &c_range);
void on_ble_command(const char *command);

#pragma region Global objects
configuration conf;
WindDirection wind_direction(COS_PIN, SIN_PIN);
WindSpeed wind_speed(SPEED_PIN);
Calibration calibration(on_calibration_complete);
BLEWind ble_wind(on_ble_command);
N2KWind n2k_wind(on_n2k_source);
ConfPersistence confPersistence;
CommandHandler cmd_handler(conf, confPersistence, calibration, ble_wind);
#pragma endregion

// ESP32 hw timer callback
inline void on_timer(unsigned long t_micros)
{
  wind_speed.loop_micros(t_micros);
  wind_direction.loop_micros(t_micros);
}

void _setup()
{
  Serial.begin(115200);
  msleep(1000);

  // read configuration from eeprom
  confPersistence.read(conf);
  WindSystem::enable_usb_tracing(conf.usb_tracing);

  // initialize wind measurement
  wind_speed.setup();
  wind_direction.setup();

  // init system (CPU freq & timers)

  WindSystem::set_timer_callback(on_timer);
  WindSystem::setup();
  
  // initialize n2k
  Log::trace("[N2K] Setting source to {%d} enabled {%d}\n", conf.n2k_source, N2K_ENABLED?1:0);
  n2k_wind.set_source(conf.n2k_source);
  n2k_wind.setup();

  // initialize ble
  ble_wind.set_device_name(conf.ble_name);
  ble_wind.setup();  

  Log::trace("[APP] Setup done\n");
}

void update_led(const wind_data &wdata)
{
  LedDriver &led = WindSystem::get_led();
  led.set_bluetooth(ble_wind.is_alive());
  led.set_error(wdata.angle_error);
  led.set_running(true);
}

void do_log(const configuration &conf, const all_data &wdata, const Wind360 &cal_progr)
{
  if (!Log::is_enabled()) return;
  
  Log::trace("[APP] Wind %s Sin/Cos {%d[%d..%d]/%d[%d..%d] %.1f} Dir {%5.1f[%5.1f]} Speed {%.1fKn/%.1fHz} Auto {%d} Err {%d %d}",
             conf.vane_type ? "ST60" : "ST50",
             wdata.wind.i_sin, conf.sin_range.low(), conf.sin_range.high(),
             wdata.wind.i_cos, conf.cos_range.low(), conf.cos_range.high(),
             wdata.wind.ellipse, wdata.wind.angle, wdata.wind.smooth_angle,
             wdata.wind.speed, wdata.wind.frequency,
             calibration.is_enabled(), wdata.wind.angle_error, wdata.n2k_err);

  if (cal_progr.size())
  {
    Log::trace(" {%02d/%02d/%02d} {", cal_progr.progress(), cal_progr.size(), cal_progr.buffer_size());
    for (int i = 0; i < cal_progr.buffer_size(); i++)
      Log::trace(" %02x", cal_progr.get_data(i));
    Log::trace(" - %.2f }", cal_progr.get_score());
  }
  Log::trace("\n");
}

void on_ble_command(const char *command)
{
  CommandResult res = cmd_handler.exec_command(command);
}

void on_n2k_source(unsigned char old_src, unsigned char new_src)
{
  Log::trace("[N2K] Source changed from {%d} to {%d}\n", old_src, new_src);
  conf.n2k_source = new_src;
  confPersistence.write(conf);
}

bool on_calibration_complete(const Range &s_range, const Range &c_range)
{
  if (s_range.is_valid() && c_range.is_valid())
  {
    conf.sin_range.set(s_range);
    conf.cos_range.set(c_range);
    if (confPersistence.write(conf))
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

void _loop()
{
  static all_data current_data;
  unsigned long t_ms = millis();
  static unsigned long t0 = t_ms;
  static unsigned long wind_n2k_t0 = t_ms;
  bool x = true;
  if (check_elapsed(t_ms, t0, MAIN_LOOP_PERIOD_LOW_FREQ))
  {
    current_data.heap = get_free_mem();

    // reload configuration
    wind_speed.apply_configuration(conf);
    wind_direction.apply_configuration(conf);
    calibration.apply_configuration(conf);
    
    // read data
    wind_direction.read_data(current_data.wind, conf,  t_ms);
    if (x) wind_speed.read_data(current_data.wind, conf, t_ms);
    x = !x;
    
    // manage calibration
    if (t_ms > CALIBRATION_SAMPLING_EXCLUSION_PERIOD) // do not sample for X seconds after restart
    {
      calibration.record_reading(current_data.wind.i_sin, current_data.wind.i_cos, current_data.wind.angle);
    }
    set_error(current_data.wind.angle_error, calibration.is_off_calibration(), WIND_ERROR_OFF_CALIBRATION);

    // send data to bluetooth
    ble_wind.send_BLE(conf, current_data, calibration);
    ble_wind.loop(t_ms);

    // send data to n2k
    if (check_elapsed(t_ms, wind_n2k_t0, WIND_N2K_DATA_FREQ)) n2k_wind.send_N2K(current_data.get_out_angle(conf), current_data.wind.speed);
    n2k_wind.loop(t_ms);
    current_data.n2k_err = n2k_wind.is_n2k_err() ? 1 : 0;

    // update led
    update_led(current_data.wind);

    WindSystem::loop(t_ms);

    // log data
    do_log(conf, current_data, calibration.get_wind360());
  }
}

#ifndef PIO_UNIT_TESTING
void setup()
{
  _setup();
}
void loop()
{
  _loop();
}
#endif

#else
#ifndef PIO_UNIT_TESTING
int main()
{
    return 0;
}
#endif
#endif// NATIVE