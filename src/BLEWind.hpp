#ifndef _BLE_WIND_H
#define _BLE_WIND_H

#include <Arduino.h>
#include <Utils.h>
#include <Log.h>
#include <BTInterface.h>
#include "WindUtil.h"
#include "DataAndConf.h"
#include "CommandHandler.h"

#define MAX_BLE_DATA_BUFFER_SIZE 128

class BLEWind: ABBLEWriteCallback
{
public:
    BLEWind(CommandHandler &h): cmd_handler(h), bt(BLE_DEVICE_UUID, BLE_DEVICE_NAME), alive(false)
    {}

   void on_write(int handle, const char* value)
    {
        if (handle==ble_command_handle)
        cmd_handler.on_command(handle, value);
    }

    void setup()
    {
        // initialize bluetooth
        ble_command_handle = bt.add_setting(BLE_COMMAND_CHARACTERISTIC_NAME, BLE_COMMAND_UUID);
        ble_conf_handle = bt.add_setting(BLE_CONF_CHARACTERISTIC_NAME, BLE_CONF_UUID);
        ble_wind_data_handle = bt.add_field(BLE_DATA_CHARACTERISTIC_NAME, BLE_WIND_DATA_UUID);
        bt.setup();
        bt.set_write_callback(this);
        bt.begin();
    }

    void send_BLE(wind_data& wdata, AutoCalibration &calib)
    {
        uint32_t mem = wdata.heap;
        uint16_t i_angle = ((int16_t)(wdata.angle * 10 + 0.5) + 3600) % 3600;  
        uint16_t i_smooth_angle = ((int16_t)(wdata.smooth_angle * 10 + 0.5) + 3600) % 3600;
        uint16_t i_output_angle = ((i_smooth_angle + wdata.conf.offset * 10)) + 3600 % 3600;
        uint16_t i_ellipse = (uint16_t)round(wdata.ellipse * 1000);
        uint16_t i_speed = isnan(wdata.speed) ? 0 : (uint16_t)(wdata.speed * 10 + 0.5);

        static ByteBuffer buffer(MAX_BLE_DATA_BUFFER_SIZE);
        buffer << i_angle << i_smooth_angle << i_output_angle << i_ellipse 
                << mem << wdata.error
                << wdata.i_sin << wdata.conf.sin_range.low() << wdata.conf.sin_range.high()
                << wdata.i_cos << wdata.conf.cos_range.low() << wdata.conf.cos_range.high()
                << i_speed << wdata.error_speed 
                << (int32_t)wdata.conf.offset
                << (uint16_t)wdata.conf.speed_adjustment 
                << wdata.conf.n2k_source
                << wdata.conf.angle_smoothing
                << wdata.conf.speed_smoothing
                << wdata.conf.calibration_score_threshold
                << wdata.conf.auto_cal
                << calib.get_wind360()
                << wdata.n2k_err
                << wdata.conf.vane_type
                << ((uint8_t)(calib.is_calibration_valid()?1:0))
                << ((uint8_t)(calib.is_calibration_in_score()?1:0));

        if (buffer.length() > MAX_BLE_DATA_BUFFER_SIZE)
        {
            Log::trace("[BLE] Data buffer overflow %d > %d\n", buffer.length(), MAX_BLE_DATA_BUFFER_SIZE);
        }
        else
        {
            bt.set_field_value(ble_wind_data_handle, buffer.data(), buffer.length());
        }

        buffer.reset();
    }

    void loop(unsigned long milli_seconds)
    {
        alive = milli_seconds < (cmd_handler.get_last_BT_activity() + 3000L); //alive activity 3s
        bt.loop(milli_seconds);
    }

    bool is_alive()
    {
        return alive;
    }

private:
    BTInterface bt;
    CommandHandler &cmd_handler;
    bool alive;

    int ble_command_handle = -1;
    int ble_conf_handle = -1;
    int ble_wind_data_handle = -1;
};

#endif