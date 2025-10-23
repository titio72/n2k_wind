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

#define MAX_BLE_DATA_BUFFER_SIZE 128
#define N2K_ENABLED false
#define SIMUL false

// 12 bit ADC for ESP32
#define MAX_ADC_VALUE 4095
#define MAX_ADC_RANGE 4096
#define RANGE_DEFAULT_MIN 1024
#define RANGE_DEFAULT_MAX 3072
#define RANGE_DEFAULT_VALID 1024

// milliseconds
#define N2K_PERIOD 500
#define BLE_PERIOD 200
// microsecond
#define MAIN_LOOP_PERIOD 500 // regulates the loop used for sampling (by the ESP32 Timer)
#define MAIN_LOOP_PERIOD_LOW_FREQ 250000L // regulates the loop used to send data on BT and N2K

#define BLE_DEVICE_UUID "32890585-c6ee-498b-9e7a-044baefb6542"
#define BLE_COMMAND_UUID "c3fe2075-ac6c-40bf-8073-73a110453725"
#define BLE_CONF_UUID "c04a9b9c-3ab6-4cce-9b59-1b582112e693"
#define BLE_WIND_DATA_UUID "003d0cab-70f7-43ac-8ab9-db26466572af"

void on_n2k_source(unsigned char old_src, unsigned char new_src);
void on_command(int handle, const char* value);

class ABWindBLEWriteCallback : public ABBLEWriteCallback
{
  void on_write(int handle, const char* value)
  {
    on_command(handle, value);
  }
} command_callback;

Conf conf(RANGE_DEFAULT_MIN, RANGE_DEFAULT_MAX, RANGE_DEFAULT_VALID);
N2K &n2k = *N2K::get_instance(NULL, on_n2k_source);
SinCosDecoder sincos_decoder;
WindDirection wind_direction(sincos_decoder, SIMUL);
WindSpeed wind_speed(SIMUL);
AnalogCalibration calibration_sin(MAX_ADC_RANGE);
AnalogCalibration calibration_cos(MAX_ADC_RANGE);
Wind360 w360;
LedDriver led;
BTInterface bt(BLE_DEVICE_UUID, "Wind");
bool calibrating = false;

hw_timer_t *timer = NULL;

int ble_command_handle = -1;
int ble_conf_handle = -1;
int ble_wind_data_handle = -1;

unsigned long last_BT_is_alive = 0;

// ESP32 hw timer callback
void IRAM_ATTR on_timer()
{
  unsigned long t = micros();
#if SIMUL == false
  wind_speed.loop_micros(t);
  wind_direction.loop_micros(t);
#endif
}

void on_n2k_source(unsigned char old_src, unsigned char new_src)
{
  Log::trace("[N2K] Source changed from {%d} to {%d}\n", old_src, new_src);
  conf.n2k_source = new_src;
  conf.write();
}

struct wind_data
{
  // angle data
  double angle = 0.0;
  double ellipse = 1.0;
  int error = WIND_ERROR_NO_CAL_OR_SIGNAL;
  uint16_t i_sin = 0;
  uint16_t i_cos = 0;
  uint16_t offset = 0;

  // speed data
  double speed = 0.0;
  int error_speed = WIND_ERROR_NO_CAL_OR_SIGNAL;
  double freq;
};

void command_set_speed_adj(const char *command_value)
{
  Log::trace("[CAL] Setting speed adjustment {%s}\n", command_value);
  if (command_value[0])
  {
    int32_t adj = 0;
    if (atoi_x(adj, command_value))
    {
      adj = adj & 0xFF; // trim to 0..255
      Log::trace("[CAL] New speed adjustment {%d}\n", adj);
      conf.speed_adjustment = adj;
      wind_speed.set_speed_adjustment(adj / 100.0);
      conf.write();
    }
    else
    {
      Log::trace("[CAL] Invalid speed adjustment\n");
    }
  }
  else
  {
    Log::trace("[CAL] Invalid speed adjustment\n");
  }
}

void command_set_offset(const char *command_value)
{
  Log::trace("[CAL] Setting offset {%s}\n", command_value);
  if (command_value[0])
  {
    int32_t offset = 0;
    if (atoi_x(offset, command_value))
    {
      Log::trace("[CAL] New offset {%d}\n", offset);
      conf.offset = offset;
      conf.write();
      sincos_decoder.set_offset(offset);
    }
    else
    {
      Log::trace("[CAL] Invalid offset\n");
    }
  }
  else
  {
    Log::trace("[CAL] Invalid offset\n");
  }
}

void command_set_calibration(const char *command_value)
{
  Log::trace("[CAL] Setting manual calibration {%s}\n", command_value);
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
  Log::trace("[CAL] Read manual cal values {%d %d %d %d}\n", vv[0], vv[1], vv[2], vv[3]);
  if (vv[0] >= 0 && vv[1] >= 0 && vv[2] >= 0 && vv[3] >= 0)
  {
    Range s_range((uint16_t)vv[0], (uint16_t)vv[1], RANGE_DEFAULT_VALID);
    Range c_range((uint16_t)vv[2], (uint16_t)vv[3], RANGE_DEFAULT_VALID);
    if (c_range.valid() && s_range.valid())
    {
      conf.sin_range.set(s_range);
      conf.cos_range.set(c_range);
      sincos_decoder.load_calibration(conf.sin_range, conf.cos_range);
      conf.write();
    }
    else
    {
      Log::trace("[CAL] Manual calibration invalid\n");
    }
  }
}

void command_finalize_calibration()
{
  // finalize calibration
  if (w360.is_valid())
  {
    Log::trace("[CAL] Complete calibration\n");
    calibrating = false;
    bool cal_sin_ok = calibration_sin.calibrate();
    bool cal_cos_ok = calibration_cos.calibrate();
    if (cal_sin_ok && cal_cos_ok)
    {
      conf.sin_range.set(calibration_sin.get_calibrated());
      conf.cos_range.set(calibration_cos.get_calibrated());
      sincos_decoder.load_calibration(conf.sin_range, conf.cos_range);
      conf.write();
    }
    else
    {
      Log::trace("[CAL] Invalid ranges to complete calibration\n");
    }
  }
  else
  {
    Log::trace("[CAL] Not enough data to complete calibration\n");
    calibrating = false;
  }
}

void command_factory_reset()
{
  Log::trace("[CAL] Reset calibration to default\n");
  conf.sin_range.set(RANGE_DEFAULT_MIN, RANGE_DEFAULT_MAX);
  conf.cos_range.set(RANGE_DEFAULT_MIN, RANGE_DEFAULT_MAX);
  conf.offset = 0;
  sincos_decoder.load_calibration(conf.sin_range, conf.cos_range);
  conf.write();
}

void command_abort_calibration()
{
  Log::trace("[CAL] Abort calibration\n");
  calibrating = false;
}

void command_start_calibration()
{
  Log::trace("[CAL] Entering calibration\n");
  calibrating = true;
  calibration_sin.reset();
  calibration_cos.reset();
  w360.reset();
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
      // just to know that BT is alive
      last_BT_is_alive = millis();
      break;
    default:
      Log::trace("[CAL] Unrecognized command {%s}\n", value);
      break;
    }
  }
}

void setup()
{
  Serial.begin(115200);
  msleep(1000);

  // initialize n2k
  if (N2K_ENABLED)
  {
    n2k.set_desired_source(conf.n2k_source);
    n2k.add_pgn(130306L);
    n2k.setup();
  }

  // initialize CPU frequency
  uint32_t f = getCpuFrequencyMhz();
  bool res_cpu_freq = setCpuFrequencyMhz(80);
  uint32_t f1 = getCpuFrequencyMhz();
  Log::trace("[APP] CPU Frequency prev {%d} new {%d}\n", f, f1);

  // read configuration from eeprom
  conf.read();
  sincos_decoder.load_calibration(conf.sin_range, conf.cos_range);
  sincos_decoder.set_offset(conf.offset);
  wind_speed.set_speed_adjustment(conf.speed_adjustment / 100.0);

  // initialize bluetooth
  ble_command_handle = bt.add_setting("command", BLE_COMMAND_UUID);
  ble_conf_handle = bt.add_setting("conf", BLE_CONF_UUID);
  ble_wind_data_handle = bt.add_field("wind", BLE_WIND_DATA_UUID);

  bt.setup();
  bt.set_write_callback(&command_callback);
  bt.begin();

  wind_speed.setup();
  wind_direction.setup();

  // initialize led
  led.setup();

  timer = timerBegin(1, 80, true);              // Timer 0, clock divider 80
  timerAttachInterrupt(timer, &on_timer, true); // Attach the interrupt handling function
  timerAlarmWrite(timer, 1000, true);           // Interrupt every 1ms
  timerAlarmEnable(timer);                      // Enable the alarm

  Log::trace("[APP] Setup done\n");
}

bool read_wind_direction(wind_data &wd)
{
  double angle = NAN;
  double ellipse = 1.0;
  int error = WIND_ERROR_OK;
  uint16_t i_sin = 0;
  uint16_t i_cos = 0;
  wind_direction.get_angle(i_sin, i_cos, angle, ellipse, error);
  wd.angle = angle;
  wd.ellipse = ellipse;
  wd.error = error;
  wd.i_cos = i_cos;
  wd.i_sin = i_sin;
  if (calibrating)
  {
    calibration_sin.add_sample(i_sin);
    calibration_cos.add_sample(i_cos);
    w360.set_degree(angle);
  }
  return error == WIND_ERROR_OK;
}

bool read_wind_speed(wind_data &wd, unsigned long t)
{
  double speed = NAN;
  double frequency = NAN;
  int error = 0;
  wind_speed.get_speed(speed, frequency, error, t);
  wd.speed = speed;
  wd.freq = frequency;
  wd.error_speed = error;
  return error == WIND_ERROR_OK;
}

void send_BLE(const wind_data &wd, Wind360 &w360, unsigned long time)
{
  static uint8_t data[MAX_BLE_DATA_BUFFER_SIZE];
  static unsigned long t0 = 0;
  if ((time - t0) < 0 || (time - t0) >= BLE_PERIOD)
  {
    t0 = time;
    uint32_t mem = get_free_mem();
    uint16_t i_angle = ((int16_t)(wd.angle * 10) + 3600) % 3600;
    uint16_t i_ellipse = (int)(wd.ellipse * 1000);
    uint16_t i_speed = isnan(wd.speed) ? 0 : (uint16_t)(wd.speed * 10 + 0.5);
    int offset = 0;
    addShort(data, offset, i_angle);
    addShort(data, offset, i_ellipse);
    addInt(data, offset, mem);
    addInt(data, offset, wd.error);
    addShort(data, offset, wd.i_sin);
    addShort(data, offset, conf.sin_range.low());
    addShort(data, offset, conf.sin_range.high());
    addShort(data, offset, wd.i_cos);
    addShort(data, offset, conf.cos_range.low());
    addShort(data, offset, conf.cos_range.high());
    addShort(data, offset, i_speed);
    addInt(data, offset, wd.error_speed);
    addInt(data, offset, conf.offset);
    addShort(data, offset, conf.speed_adjustment);

    if (calibrating)
    {
      addChar(data, offset, w360.size());
      for (int i = 0; i < w360.buffer_size(); i++)
      {
        addChar(data, offset, w360.get_data(i));
      }
    }
    else
    {
      addChar(data, offset, 0); // no calibration data
    }
    bt.set_field_value(ble_wind_data_handle, data, offset); // hope offset<MAX_BLE_DATA_BUFFER_SIZE
  }
}

void send_N2K(const wind_data &wd, unsigned long time)
{
  if (N2K_ENABLED)
  {
    static unsigned long t0 = 0;
    if ((time - t0) < 0 || (time - t0) >= N2K_PERIOD)
    {
      t0 = time;
      if (wd.error == WIND_ERROR_OK)
      {
        tN2kMsg msg(n2k.get_source());
        SetN2kWindSpeed(msg, 0, KnotsToms(wd.speed), DegToRad(wd.angle), tN2kWindReference::N2kWind_Apparent);
        n2k.send_msg(msg);
      }
    }
  }
}

void update_led(const wind_data &wd, unsigned long t)
{
  if (t<last_BT_is_alive + 3000L)
    led.set_blue(true);
  else
    led.set_blue(false);

  led.set_error(wd.error);
  led.set_calibration(calibrating);
}

void loop()
{
  static wind_data wdata;
  unsigned long t = micros();

#if SIMUL == true
  wind_speed.set_apparent_wind_angle(wdata.angle);
  wind_speed.loop_micros(t);
  wind_direction.loop_micros(t);
#endif

  static unsigned long t0 = t;

  if ((t - t0) > MAIN_LOOP_PERIOD_LOW_FREQ || (t < t0)) // 5/10 second
  {
    t0 = t;
    unsigned long t_ms = t / 1000L;

    wdata.offset = conf.offset;

    // read wind
    read_wind_direction(wdata);
    read_wind_speed(wdata, t);

    Log::trace("[APP] Wind Sin/Cos {%d[%d..%d]/%d[%d..%d]} Dir {%5.1f} Ellipse {%.1f} ErrCode {%d} Speed {%.1f} Freq {%.1f} ErrCodeSpeed {%d}",
               wdata.i_sin, conf.sin_range.low(), conf.cos_range.high(), wdata.i_cos, conf.cos_range.low(), conf.cos_range.high(),
               wdata.angle, wdata.ellipse, wdata.error,
               wdata.speed, wdata.freq, wdata.error_speed);

    if (calibrating)
    {
      Log::trace("{%02d/%02d/%02d} {", w360.progress(), w360.size(), w360.buffer_size());
      for (int i = 0; i < w360.buffer_size(); i++)
        Log::trace(" %02x", w360.get_data(i));
      Log::trace("}");
    }
    Log::trace("                \r");

    // update led
    update_led(wdata, t_ms);
    led.loop(t_ms);

    // send data to bluetooth
    send_BLE(wdata, w360, t_ms);
    bt.loop(t_ms);

    // send data to n2k
    send_N2K(wdata, t_ms);
    n2k.loop(t_ms);
  }
}
