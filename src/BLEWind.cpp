#include <Log.h>
#include "BLEWind.h"
#include "WindUtil.h"
#include "DataAndConf.h"
#include "Calibration.h"
#include "BLESerializer.h"
#include "CorrectionProtocol.h"
#include <Utils.h>

#define MAX_BLE_DATA_BUFFER_SIZE 128

BLEWind::BLEWind(on_command_callback cback): 
    bt(BLE_DEVICE_UUID, BLE_DEVICE_NAME, this), alive(false), buffer(MAX_BLE_DATA_BUFFER_SIZE), command_callback(cback)
{}

void BLEWind::on_write(int handle, const char* value)
{
    last_BT_is_alive = _millis();
    if (handle==ble_command_handle && command_callback) command_callback(value);
}

void BLEWind::setup()
{
    // initialize bluetooth
    ble_command_handle = bt.add_setting(BLE_COMMAND_CHARACTERISTIC_NAME, BLE_COMMAND_UUID);
    ble_conf_handle = bt.add_setting(BLE_CONF_CHARACTERISTIC_NAME, BLE_CONF_UUID);
    ble_wind_data_handle = bt.add_field(BLE_DATA_CHARACTERISTIC_NAME, BLE_WIND_DATA_UUID);
    // notify, not indicate: set_correction_table() must not wait for an ack
    // (nobody is expected to subscribe, see BLE_TABLE_UUID)
    ble_table_handle = bt.add_field(BLE_TABLE_CHARACTERISTIC_NAME, BLE_TABLE_UUID, true);
    bt.setup();
    bt.begin();
}

void BLEWind::send_BLE(const configuration& conf, const all_data &wdata, const Calibration &calib)
{
    if (!alive)
        return; // nobody is listening - save power

    buffer.reset();
    make_message(conf, wdata, calib, buffer);

    if (buffer.length() > MAX_BLE_DATA_BUFFER_SIZE)
    {
        Log::trace("[BLE] Data buffer overflow %d > %d\n", buffer.length(), MAX_BLE_DATA_BUFFER_SIZE);
    }
    else
    {
        bt.set_field_value(ble_wind_data_handle, buffer.data(), buffer.length());
    }
}

void BLEWind::set_correction_table(const correction_table &t)
{
    // Set whether or not anybody is connected. It is read, not streamed: pushing it makes the
    // stack send it on the live connection, and the data indications stop on that connection.
    uint8_t payload[CORRECTION_MAX_PAYLOAD];
    size_t len = make_table_message(t, payload, sizeof(payload));
    if (len == 0)
        Log::trace("[BLE] Correction table does not fit\n");
    else
        bt.store_field_value(ble_table_handle, payload, (int)len);
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