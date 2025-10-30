#define N2K_DEVICE_FUNCTION 180
#define N2K_DEVICE_NAME "ABN2kWind                       "
#define N2K_DEVICE_CODE 101
#define N2K_DEVICE_SERIAL_CODE "00000002"
#define N2K_SOFTWARE_VERSION "0.0.1"
#define N2K_DEVICE_VERSION "0.0.1"
#define N2K_DEVICE_SERIAL 1

#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include <Log.h>
#include <N2K.h>
#include <EEPROM.h>
#include <BTInterface.h>
#include <N2K.h>
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

#define MAX_BLE_DATA_BUFFER_SIZE 128
#define N2K_ENABLED false

// microsecond
#define MAIN_LOOP_PERIOD_LOW_FREQ 200000L // regulates the loop used to send data on BT and N2K

#define BLE_DEVICE_UUID "32890585-c6ee-498b-9e7a-044baefb6542"
#define BLE_COMMAND_UUID "c3fe2075-ac6c-40bf-8073-73a110453725"
#define BLE_CONF_UUID "c04a9b9c-3ab6-4cce-9b59-1b582112e693"
#define BLE_WIND_DATA_UUID "003d0cab-70f7-43ac-8ab9-db26466572af"

void on_n2k_source(unsigned char old_src, unsigned char new_src);
void on_command(int handle, const char* value);
void on_calibration_complete(Range &s_range, Range &c_range);

#pragma region Global objects
wind_data wdata;
Conf conf(RANGE_DEFAULT_MIN, RANGE_DEFAULT_MAX, RANGE_DEFAULT_VALID);
N2K &n2k = *N2K::get_instance(NULL, on_n2k_source);
Range sin_range;
Range cos_range;
SinCosDecoder sincos_decoder(sin_range, cos_range);
WindDirection wind_direction(sincos_decoder, wdata);
WindSpeed wind_speed(wdata);

AutoCalibration auto_calibration(on_calibration_complete);
ManualCalibration manual_calibration(on_calibration_complete);

LedDriver led;
BTInterface bt(BLE_DEVICE_UUID, "Wind");

hw_timer_t *timer = NULL;

int ble_command_handle = -1;
int ble_conf_handle = -1;
int ble_wind_data_handle = -1;

unsigned long last_BT_is_alive = 0;
#pragma endregion

// ESP32 hw timer callback
void IRAM_ATTR on_timer()
{
  unsigned long t = micros();
  wind_speed.loop_micros(t);
  wind_direction.loop_micros(t);
}

void on_n2k_source(unsigned char old_src, unsigned char new_src)
{
  Log::trace("[N2K] Source changed from {%d} to {%d}\n", old_src, new_src);
  conf.n2k_source = new_src;
  conf.write();
}

#pragma region BLE command handlers
class ABWindBLEWriteCallback : public ABBLEWriteCallback
{
  void on_write(int handle, const char* value)
  {
    on_command(handle, value);
  }
} command_callback;

void command_set_speed_adj(const char *command_value)
{
  Log::trace("[CMD] Setting speed adjustment {%s}\n", command_value);
  if (command_value[0])
  {
    int32_t adj = 0;
    if (atoi_x(adj, command_value))
    {
      uint32_t uiAdj = adj & 0xFF; // trim to 0..255
      Log::trace("[CMD] New speed adjustment {%d}\n", uiAdj);
      conf.speed_adjustment = uiAdj;
      wind_speed.set_speed_adjustment(adj / 100.0);
      conf.write();
    }
    else
    {
      Log::trace("[CMD] Invalid speed adjustment\n");
    }
  }
  else
  {
    Log::trace("[CMD] Invalid speed adjustment\n");
  }
}

void command_set_offset(const char *command_value)
{
  Log::trace("[CMD] Setting offset {%s}\n", command_value);
  if (command_value[0])
  {
    int32_t offset = 0;
    if (atoi_x(offset, command_value))
    {
      Log::trace("[CMD] New offset {%d}\n", offset);
      conf.offset = offset;
      conf.write();
      sincos_decoder.set_offset(offset);
    }
    else
    {
      Log::trace("[CMD] Invalid offset\n");
    }
  }
  else
  {
    Log::trace("[CMD] Invalid offset\n");
  }
}

void command_set_calibration(const char *command_value)
{
  Log::trace("[CMD] Setting manual calibration {%s}\n", command_value);
  int i_tok = 0;
  int32_t vv[] = {-1, -1, -1, -1};
  bool do_write = false;
  char s[256];
  strcpy(s, command_value);
  char *t, *p;
  for (t = mystrtok(&p, s, '|'); t && i_tok < 4; t = mystrtok(&p, 0, '|'))
  {
    if (strlen(t))
    {
      parse_value(vv[i_tok], t, MAX_ADC_VALUE);
    }
    i_tok++;
  }
  Log::trace("[CMD] Read manual cal values {%d %d %d %d}\n", vv[0], vv[1], vv[2], vv[3]);
  if (vv[0] >= 0 && vv[1] >= 0 && vv[2] >= 0 && vv[3] >= 0)
  {
    Range s_range((uint16_t)vv[0], (uint16_t)vv[1], RANGE_DEFAULT_VALID);
    Range c_range((uint16_t)vv[2], (uint16_t)vv[3], RANGE_DEFAULT_VALID);
    if (c_range.valid() && s_range.valid())
    {
      conf.sin_range = s_range;
      conf.cos_range = c_range;
      sin_range = s_range;
      cos_range = c_range;
      conf.write();
    }
    else
    {
      Log::trace("[CMD] Manual calibration invalid\n");
    }
  }
}

void command_finalize_calibration()
{
  Log::trace("[CMD] finalize calibration\n");
  manual_calibration.finalize();
}

void command_factory_reset()
{
  Log::trace("[CMD] Reset calibration to default\n");
  conf.sin_range.set(RANGE_DEFAULT_MIN, RANGE_DEFAULT_MAX);
  conf.cos_range.set(RANGE_DEFAULT_MIN, RANGE_DEFAULT_MAX);
  conf.offset = 0;
  sin_range = conf.sin_range;
  cos_range = conf.cos_range;
  conf.write();
}

void command_abort_calibration()
{
  Log::trace("[CMD] Abort calibration\n");
  manual_calibration.abort();
}

void command_start_calibration()
{
  Log::trace("[CMD] Starting calibration\n");
  manual_calibration.start();
  auto_calibration.disable();
}

void command_set_speed_smoothing(const char *command_value)
{
  Log::trace("[CAL] Setting speed smoothing {%s}\n", command_value);
  if (command_value[0])
  {
    int32_t smoothing = 0;
    if (atoi_x(smoothing, command_value))
    {
      uint32_t uSmoothing = smoothing & 0xFF; // trim to 0..255
      double alpha = (double)uSmoothing / 50.0; // map to 0..50
      Log::trace("[CAL] New speed smoothing {%d} alpha {%.2f}\n", uSmoothing, alpha);
      conf.speed_smoothing = uSmoothing;
      conf.write();
      wdata.speed_smoothing_factor = alpha;
    }
    else
    {    
      Log::trace("[CAL] Invalid speed smoothing\n");
    }
  }
}

void command_set_angle_smoothing(const char *command_value)
{
  Log::trace("[CAL] Setting angle smoothing {%s}\n", command_value);
  if (command_value[0])
  {
    int32_t smoothing = 0;
    if (atoi_x(smoothing, command_value))
    {
      uint32_t uSmoothing = smoothing & 0xFF; // trim to 0..255
      double alpha = (double)uSmoothing / 50.0; // the actual range is 0..50
      Log::trace("[CAL] New angle smoothing {%d} alpha {%.2f}\n", uSmoothing, alpha);
      conf.angle_smoothing = uSmoothing;
      conf.write();
      wdata.angle_smoothing_factor = alpha;
    }
    else
    {    
      Log::trace("[CAL] Invalid angle smoothing\n");
    }
  }
}

void on_command(int handle, const char *value)
{
  if (handle == ble_command_handle && value && value[0])
  {
    const char command = value[0];
    const char *command_value = (value + sizeof(char));
    switch (command)
    {
    case 'K': // Set speed adj
      command_set_speed_adj(command_value);
      break;
    case 'O': // Set offset
      command_set_offset(command_value);
      break;
    case 'S': // Set manual calibration
      command_set_calibration(command_value);
      break;
    case 'A': // Abort calibration
      command_abort_calibration();
      break;
    case 'R': // Finalize calibration
      command_finalize_calibration();
      break;
    case 'C': // Start calibration
      command_start_calibration();
      break;
    case 'X': // Calibration factory reset
      command_factory_reset();
      break;
    case 'H': // Heartbeat
      last_BT_is_alive = millis();
      break;
    case 'W':
      command_set_speed_smoothing(command_value);
      break;
    case 'Q':
      command_set_angle_smoothing(command_value);
      break;
    case 'P': // Start auto calibration
      Log::trace("[CAL] Toggle auto calibration {%s}\n", auto_calibration.is_enabled() ? "OFF" : "ON");
      if (auto_calibration.is_enabled())
      {
        auto_calibration.disable(); 
      }
      else
      {
        auto_calibration.enable();
        manual_calibration.abort();
      }
      break;
    default:
      Log::trace("[CAL] Unrecognized command {%s}\n", value);
      break;
    }
  }
}
#pragma endregion

void setup()
{
  Serial.begin(115200);
  msleep(1000);

  // initialize CPU frequency
  uint32_t f = getCpuFrequencyMhz();
  bool res_cpu_freq = setCpuFrequencyMhz(80);
  uint32_t f1 = getCpuFrequencyMhz();
  Log::trace("[APP] CPU Frequency prev {%d} new {%d}\n", f, f1);

  // initialize n2k
  if (N2K_ENABLED)
  {
    n2k.set_desired_source(conf.n2k_source);
    n2k.add_pgn(130306L);
    n2k.setup();
  } 
  
  // read configuration from eeprom
  conf.read();
  sin_range = conf.sin_range;
  cos_range = conf.cos_range;
  sincos_decoder.set_offset(conf.offset);
  wind_speed.set_speed_adjustment(conf.speed_adjustment / 100.0);
  wdata.angle_smoothing_factor = (double)conf.angle_smoothing / 50.0; if (wdata.angle_smoothing_factor>1.0) wdata.angle_smoothing_factor=1.0;
  wdata.speed_smoothing_factor = (double)conf.speed_smoothing / 50.0; if (wdata.speed_smoothing_factor>1.0) wdata.speed_smoothing_factor=1.0;  

  // initialize bluetooth
  ble_command_handle = bt.add_setting("command", BLE_COMMAND_UUID);
  ble_conf_handle = bt.add_setting("conf", BLE_CONF_UUID);
  ble_wind_data_handle = bt.add_field("wind", BLE_WIND_DATA_UUID);
  bt.setup();
  bt.set_write_callback(&command_callback);
  bt.begin();

  // initialize wind measurement
  wind_speed.setup();
  wind_direction.setup();

  // initialize led
  led.setup();

  // initialize hw timer for wind measurement
  timer = timerBegin(1, 80, true);              // Timer 0, clock divider 80
  timerAttachInterrupt(timer, &on_timer, true); // Attach the interrupt handling function
  timerAlarmWrite(timer, 1000, true);           // Interrupt every 1ms
  timerAlarmEnable(timer);                      // Enable the alarm

  Log::trace("[APP] Setup done\n");
}

void appendW360(Wind360 &w, uint8_t *data, int &offset)
{
    addChar(data, offset, w.size());
    for (int i = 0; i < w.buffer_size(); i++)
    {
      addChar(data, offset, w.get_data(i));
    }
}

void send_BLE(unsigned long time)
{
  static uint8_t data[MAX_BLE_DATA_BUFFER_SIZE];
  uint32_t mem = get_free_mem();
  uint16_t i_angle = ((int16_t)(wdata.angle * 10) + 3600) % 3600;  
  uint16_t i_smooth_angle = ((int16_t)(wdata.smooth_angle * 10) + 3600) % 3600;
  uint16_t i_ellipse = (int)(wdata.ellipse * 1000);
  uint16_t i_speed = isnan(wdata.speed) ? 0 : (uint16_t)(wdata.speed * 10 + 0.5);
  int offset = 0;
  addShort(data, offset, i_angle);
  addShort(data, offset, i_smooth_angle);
  addShort(data, offset, i_ellipse);
  addInt(data, offset, mem);
  addInt(data, offset, wdata.error);
  addShort(data, offset, wdata.i_sin);
  addShort(data, offset, conf.sin_range.low());
  addShort(data, offset, conf.sin_range.high());
  addShort(data, offset, wdata.i_cos);
  addShort(data, offset, conf.cos_range.low());
  addShort(data, offset, conf.cos_range.high());
  addShort(data, offset, i_speed);
  addInt(data, offset, wdata.error_speed);
  addInt(data, offset, conf.offset);
  addShort(data, offset, conf.speed_adjustment);
  addChar(data, offset, conf.n2k_source); // new
  addChar(data, offset, conf.angle_smoothing); // new
  addChar(data, offset, conf.speed_smoothing); // new
  addChar(data, offset, auto_calibration.is_enabled() ? 1 : 0); // new

  if (manual_calibration.is_in_progress()) appendW360(manual_calibration.get_wind360(), data, offset);
  else if (auto_calibration.is_enabled()) appendW360(auto_calibration.get_wind360(), data, offset);
  else addChar(data, offset, 0); // no calibration data

  bt.set_field_value(ble_wind_data_handle, data, offset); // hope offset<MAX_BLE_DATA_BUFFER_SIZE
}

void send_N2K(unsigned long time)
{
  if (N2K_ENABLED)
  {
    if (wdata.error == WIND_ERROR_OK)
    {
      tN2kMsg msg(n2k.get_source());
      SetN2kWindSpeed(msg, 0, KnotsToms(wdata.speed), DegToRad(wdata.smooth_angle), tN2kWindReference::N2kWind_Apparent);
      n2k.send_msg(msg);
    }
  }
}

void update_led(unsigned long t)
{
  if (t<last_BT_is_alive + 3000L)
    led.set_blue(true);
  else
    led.set_blue(false);

  led.set_error(wdata.error);
  led.set_calibration(manual_calibration.is_in_progress());
}

void do_log()
{
    Log::trace("[APP] Wind Sin/Cos {%d[%d..%d]/%d[%d..%d]} Dir {%5.1f} Ellipse {%.1f} ErrCode {%d} Speed {%.1f} Freq {%.1f} ErrCodeSpeed {%d}",
               wdata.i_sin, conf.sin_range.low(), conf.cos_range.high(), wdata.i_cos, conf.cos_range.low(), conf.cos_range.high(),
               wdata.angle, wdata.ellipse, wdata.error,
               wdata.speed, wdata.frequency, wdata.error_speed);

    if (manual_calibration.is_in_progress())
    {
      Wind360 &w360 = manual_calibration.get_wind360();
      Log::trace("{%02d/%02d/%02d} {", w360.progress(), w360.size(), w360.buffer_size());
      for (int i = 0; i < w360.buffer_size(); i++)
        Log::trace(" %02x", w360.get_data(i));
      Log::trace("}");
    }
    Log::trace("                \r\n");
}

void on_calibration_complete(Range &s_range, Range &c_range)
{
    sin_range = s_range;
    cos_range = c_range;
    conf.sin_range = s_range;
    conf.cos_range = c_range;
    conf.write();
    Log::trace("[CAL] Calibration updated : sin {%d %d} cos {%d %d}\n",
               conf.sin_range.low(), conf.sin_range.high(),
               conf.cos_range.low(), conf.cos_range.high());
}

void loop()
{
  unsigned long t = micros();
  static unsigned long t0 = t;
  if (check_elapsed(t, t0, MAIN_LOOP_PERIOD_LOW_FREQ))
  {
    unsigned long t_ms = t / 1000L;

    wdata.offset = conf.offset;

    // read wind
    wind_direction.loop(t_ms);
    wind_speed.loop(t_ms);

    // manage calibration
    auto_calibration.record_reading(wdata.i_sin, wdata.i_cos, wdata.angle);
    manual_calibration.record_reading(wdata.i_sin, wdata.i_cos, wdata.angle);

    // update led
    update_led(t_ms);
    led.loop(t_ms);

    // send data to bluetooth
    send_BLE(t_ms);
    bt.loop(t_ms);

    // send data to n2k
    send_N2K(t_ms);
    n2k.loop(t_ms);

    do_log();
  }
}
