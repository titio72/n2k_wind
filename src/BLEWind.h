#ifndef _BLE_WIND_H
#define _BLE_WIND_H
#include <BTInterface.h>
#include "WindUtil.h"

class Calibration;
struct wind_data;

typedef void (*on_command_callback)(const char *command);

class BLEWind: ABBLEWriteCallback
{
public:
    BLEWind(on_command_callback cback);

   void on_write(int handle, const char* value);

    void setup();

    void send_BLE(const wind_data& wdata, const Calibration &calib);

    void loop(unsigned long milli_seconds);

    bool is_alive() const;

    void set_device_name(const char* name);

private:
    BTInterface bt;
    bool alive;
    ByteBuffer buffer;
    int ble_command_handle = -1;
    int ble_conf_handle = -1;
    int ble_wind_data_handle = -1;
    on_command_callback command_callback;
    unsigned long last_BT_is_alive = 0;
};

#endif