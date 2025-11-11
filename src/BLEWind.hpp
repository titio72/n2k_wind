#ifndef _BLE_WIND_H
#define _BLE_WIND_H

#include <Arduino.h>
#include <Utils.h>
#include <Log.h>
#include <BTInterface.h>
#include "WindUtil.h"
#include "Conf.h"
#include "CommandHandler.h"

#define BLE_DEVICE_UUID "32890585-c6ee-498b-9e7a-044baefb6542"
#define BLE_COMMAND_UUID "c3fe2075-ac6c-40bf-8073-73a110453725"
#define BLE_CONF_UUID "c04a9b9c-3ab6-4cce-9b59-1b582112e693"
#define BLE_WIND_DATA_UUID "003d0cab-70f7-43ac-8ab9-db26466572af"

#define MAX_BLE_DATA_BUFFER_SIZE 128

class BLEWind: ABBLEWriteCallback
{
public:
    BLEWind(CommandHandler &h): cmd_handler(h), bt(BLE_DEVICE_UUID, "Wind"), alive(false)
    {}

   void on_write(int handle, const char* value)
    {
        if (handle==ble_command_handle)
        cmd_handler.on_command(handle, value);
    }

    void setup()
    {
        // initialize bluetooth
        ble_command_handle = bt.add_setting("command", BLE_COMMAND_UUID);
        ble_conf_handle = bt.add_setting("conf", BLE_CONF_UUID);
        ble_wind_data_handle = bt.add_field("wind", BLE_WIND_DATA_UUID);
        bt.setup();
        bt.set_write_callback(this);
        bt.begin();
    }

    void send_BLE(wind_data& wdata, Conf &conf, Wind360 &calib_progress)
    {
        uint32_t mem = get_free_mem();
        uint16_t i_angle = ((int16_t)(wdata.angle * 10) + 3600) % 3600;  
        uint16_t i_smooth_angle = ((int16_t)(wdata.smooth_angle * 10) + 3600) % 3600;
        uint16_t i_ellipse = (int)(wdata.ellipse * 1000);
        uint16_t i_speed = isnan(wdata.speed) ? 0 : (uint16_t)(wdata.speed * 10 + 0.5);

        static ByteBuffer buffer(MAX_BLE_DATA_BUFFER_SIZE);
        buffer << i_angle << i_smooth_angle << i_ellipse 
                << mem << wdata.error
                << wdata.i_sin << conf.sin_range.low() << conf.sin_range.high()
                << wdata.i_cos << conf.cos_range.low() << conf.cos_range.high()
                << i_speed << wdata.error_speed 
                << (int32_t)conf.offset
                << (uint16_t)conf.speed_adjustment 
                << conf.n2k_source
                << conf.angle_smoothing << conf.speed_smoothing
                << conf.calibration_score_threshold
                << conf.auto_cal
                << calib_progress;

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
    CommandHandler cmd_handler;
    bool alive;

    int ble_command_handle = -1;
    int ble_conf_handle = -1;
    int ble_wind_data_handle = -1;
};

#endif