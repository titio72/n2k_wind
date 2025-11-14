#ifndef _CONSTANTS_H
#define _CONSTANTS_H

// 12 bit ADC for ESP32
#define MAX_ADC_VALUE 4095
#define MAX_ADC_RANGE 4096
#define RANGE_DEFAULT_MIN 1024
#define RANGE_DEFAULT_MAX 3072
#define RANGE_DEFAULT_VALID 512

#define WIND_ERROR_NO_CAL_OR_SIGNAL 1
#define WIND_ERROR_OK 0

#ifndef DEFAULT_WIND_N2K_SOURCE
#define DEFAULT_WIND_N2K_SOURCE 32
#endif

/*
On ST50:
The vane are r=55mm from the center, so a full round is 2*pi*r
With a frequency of f HZ (there are 4 magnets, so 2 transitions per round, hence the round per seconds are f/2),
it makes a total distance of L=(f/2)*2*pi*r in 1 second, hence the speed in knots is L*3600/1852.
This is assuming that there is no friction - let's assume that the rotation speed is 50% of.
The magic number to convert Hz in Knots is 2*(pi*0.055*2600/1852) = 0.672

On ST60 
Raymarine says that 20Hz (10Hz considering a full revolution) is 20Knots, which would mean a factor of 1.0 instead of 0.672
*/
#if VANE_TYPE==ST50
#define HZ_TO_KNOTS 0.672f // use 0.672f for ST50
#else
#define HZ_TO_KNOTS 1.000f // use 1.000f for ST60
#endif

// microsecond
#define MAIN_LOOP_PERIOD_LOW_FREQ 250000L // regulates the main loop used to read sensors and interacts with N2K & BLE
#define WIND_N2K_DATA_FREQ 500000L // regulates how frequently send out wind info on the N2K bus
#define CALIBRATION_SAMPLING_EXCLUSION_PERIOD 30000L // milliseconds - do not take samples for 30 seconds after restart (sample would be funny)
#define CPU_FREQUENCY 80

#define BLE_DEVICE_UUID "32890585-c6ee-498b-9e7a-044baefb6542"
#define BLE_COMMAND_UUID "c3fe2075-ac6c-40bf-8073-73a110453725"
#define BLE_CONF_UUID "c04a9b9c-3ab6-4cce-9b59-1b582112e693"
#define BLE_WIND_DATA_UUID "003d0cab-70f7-43ac-8ab9-db26466572af"

#define BLE_DEVICE_NAME "Wind"
#define BLE_COMMAND_CHARACTERISTIC_NAME "command"
#define BLE_DATA_CHARACTERISTIC_NAME "data"
#define BLE_CONF_CHARACTERISTIC_NAME "conf"
#endif