#include <Arduino.h>
#include <math.h>
#include <Log.h>

#include "SinCosDecoder.h"
#include "Wind360.h"
#include "AnalogCalibration.h"
#include "WindSpeed.h"
#include "WindDirection.h"
#include "LedDriver.h"
#include "WindUtil.h"
#include "Conf.h"
#include "AutoCalibration.h"
#include "ManualCalibration.h"
#include "CommandHandler.h"
#include "BLEWind.hpp"
#include "N2kWind.hpp"
#include "WindSystem.h"



void on_n2k_source(unsigned char old_src, unsigned char new_src);
void on_calibration_complete(Range &s_range, Range &c_range);

#pragma region Global objects
Conf conf;
SinCosDecoder sincos_decoder;
WindDirection wind_direction(sincos_decoder);
WindSpeed wind_speed;
AutoCalibration auto_calibration(on_calibration_complete);
ManualCalibration manual_calibration(on_calibration_complete);
LedDriver led;
CommandContext context = {conf, auto_calibration, manual_calibration};
CommandHandler cmd_handler(context);
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
  static Wind360 emptyW60(0);
  if (manual_calibration.is_in_progress())
    return manual_calibration.get_wind360();
  else if (auto_calibration.is_enabled())
    return auto_calibration.get_wind360();
  else
    return emptyW60;
}

void update_led(const wind_data &wdata)
{
  led.set_blue(ble_wind.is_alive());
  led.set_error(wdata.error);
  led.set_calibration(manual_calibration.is_in_progress());
}

void do_log(const wind_data &wdata)
{
  Log::trace("[APP] Wind Sin/Cos {%d[%d..%d]/%d[%d..%d]} Dir {%5.1f[%5.1f]} Ellipse {%.1f} ErrCode {%d} Speed {%.1f} Freq {%.1f} ErrCodeSpeed {%d} Auto {%d}",
             wdata.i_sin, conf.sin_range.low(), conf.cos_range.high(), wdata.i_cos, conf.cos_range.low(), conf.cos_range.high(),
             wdata.angle, wdata.smooth_angle, wdata.ellipse, wdata.error,
             wdata.speed, wdata.frequency, wdata.error_speed,
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

void on_calibration_complete(Range &s_range, Range &c_range)
{
  conf.sin_range.set(s_range);
  conf.cos_range.set(c_range);
  conf.write();
  Log::trace("[CAL] Calibration updated : sin {%d %d} cos {%d %d}\n",
             conf.sin_range.low(), conf.sin_range.high(),
             conf.cos_range.low(), conf.cos_range.high());
}

void loop()
{
  static wind_data wdata;
  unsigned long t = micros();
  static unsigned long t0 = t;
  static unsigned long n2k_t0 = t;
  if (check_elapsed(t, t0, MAIN_LOOP_PERIOD_LOW_FREQ))
  {
    unsigned long t_ms = t / 1000L;

    // reload configuration
    wdata.speed_smoothing_factor = conf.get_speed_smoothing_factor();
    wdata.angle_smoothing_factor = conf.get_angle_smoothing_factor();
    wdata.offset = conf.offset;
    wdata.calibration_score_threshold = conf.calibration_score_threshold;
    sincos_decoder.apply_configuration(conf);
    wind_speed.apply_configuration(conf);
    auto_calibration.apply_configuration(conf);

    // read wind
    wind_direction.read_data(wdata, t_ms);
    wind_speed.read_data(wdata, t_ms);

    // manage calibration
    auto_calibration.record_reading(wdata.i_sin, wdata.i_cos, wdata.angle);
    manual_calibration.record_reading(wdata.i_sin, wdata.i_cos, wdata.angle);

    // update led
    update_led(wdata);
    led.loop(t_ms);

    // send data to bluetooth
    ble_wind.send_BLE(wdata, conf, get_calibration_progress());
    ble_wind.loop(t_ms);

    // send data to n2k
    if (check_elapsed(t, n2k_t0, WIND_N2K_DATA_FREQ)) n2k_wind.send_N2K(wdata, t_ms);
    n2k_wind.loop(t_ms);

    wdata.n2k_err = n2k_wind.is_n2k_err()?1:0;

    do_log(wdata);
  }
}
