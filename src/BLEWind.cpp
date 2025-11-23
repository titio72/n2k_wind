#include <Arduino.h>
#include <Log.h>
#include "BLEWind.h"
#include "WindUtil.h"
#include "DataAndConf.h"
#include "Calibration.h"

#define MAX_BLE_DATA_BUFFER_SIZE 128

const uint8_t YES = 1;
const uint8_t NO = 0;

BLEWind::BLEWind(on_command_callback cback): 
    bt(BLE_DEVICE_UUID, BLE_DEVICE_NAME), alive(false), buffer(MAX_BLE_DATA_BUFFER_SIZE), command_callback(cback)
{}

void BLEWind::on_write(int handle, const char* value)
{
    last_BT_is_alive = millis();
    if (handle==ble_command_handle && command_callback) command_callback(value);
}

void BLEWind::setup()
{
    // initialize bluetooth
    ble_command_handle = bt.add_setting(BLE_COMMAND_CHARACTERISTIC_NAME, BLE_COMMAND_UUID);
    ble_conf_handle = bt.add_setting(BLE_CONF_CHARACTERISTIC_NAME, BLE_CONF_UUID);
    ble_wind_data_handle = bt.add_field(BLE_DATA_CHARACTERISTIC_NAME, BLE_WIND_DATA_UUID);
    bt.setup();
    bt.set_write_callback(this);
    bt.begin();
}

void BLEWind::send_BLE(const wind_data& wdata, const Calibration &calib)
{
    uint32_t mem = wdata.heap;
    uint16_t i_angle = ((int16_t)(wdata.angle * 10 + 0.5) + 3600) % 3600;  
    uint16_t i_smooth_angle = ((int16_t)(wdata.smooth_angle * 10 + 0.5) + 3600) % 3600;
    uint16_t i_output_angle = ((i_smooth_angle + wdata.conf.offset * 10) + 3600) % 3600;
    uint16_t i_ellipse = (uint16_t)round(wdata.ellipse * 1000);
    uint16_t i_speed = isnan(wdata.speed) ? 0 : (uint16_t)(wdata.speed * 10 + 0.5);

    buffer.reset();
    buffer << i_angle << i_smooth_angle << i_output_angle << i_ellipse 
            << mem << wdata.angle_error
            << wdata.i_sin << wdata.conf.sin_range.low() << wdata.conf.sin_range.high()
            << wdata.i_cos << wdata.conf.cos_range.low() << wdata.conf.cos_range.high()
            << i_speed << wdata.speed_error 
            << wdata.conf.offset
            << wdata.conf.speed_adjustment 
            << wdata.conf.n2k_source
            << wdata.conf.angle_smoothing
            << wdata.conf.speed_smoothing
            << wdata.conf.calibration_score_threshold
            << wdata.conf.auto_cal
            << calib.get_wind360()
            << wdata.n2k_err
            << wdata.conf.vane_type
            << (calib.is_calibration_valid()?YES:NO)
            << (calib.is_calibration_in_score()?YES:NO)
            << (Log::is_enabled()?YES:NO);

    if (buffer.length() > MAX_BLE_DATA_BUFFER_SIZE)
    {
        Log::trace("[BLE] Data buffer overflow %d > %d\n", buffer.length(), MAX_BLE_DATA_BUFFER_SIZE);
    }
    else
    {
        bt.set_field_value(ble_wind_data_handle, buffer.data(), buffer.length());
    }
}

void BLEWind::loop(unsigned long milli_seconds)
{
    alive = milli_seconds < (last_BT_is_alive + BLE_ACTIVITY_TIMEOUT); //alive activity 3s
    bt.loop(milli_seconds);
}

bool BLEWind::is_alive() const
{
    return alive;
}

void BLEWind::set_device_name(const char* name)
{
    bt.set_device_name(name);
}