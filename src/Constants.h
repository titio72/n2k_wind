#ifndef _CONSTANTS_H
#define _CONSTANTS_H

// 12 bit ADC for ESP32, 16 bit ADC for ADS1115
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
#define VANE_TYPE_ST50 0
#define VANE_TYPE_ST60 1

#define VANE_TYPE_DEFAULT VANE_TYPE_ST50 // ST50

#define HZ_TO_KNOTS_ST50 0.672 // use 0.672 for ST50
#define HZ_TO_KNOTS_ST60 1.000 // use 1.000 for ST60

// Sin and Cos are calculated as moving average over N samples
#define SIN_COS_BUFFER_SIZE 200 // about 0.2s averaging at 1ms rate 

// main loop
#define MAIN_LOOP_PERIOD_LOW_FREQ 200000L // (micros) regulates the main loop used to read sensors and interacts with N2K & BLE
#define WIND_N2K_DATA_FREQ 500000L // (micros) regulates how frequently send out wind info on the N2K bus
#define CALIBRATION_SAMPLING_EXCLUSION_PERIOD 30000L // (millis) do not take samples for 30 seconds after restart (sample would be funny)
#define CPU_FREQUENCY 80 // MHz (ESP32 default is 160). 80 is the lower value that makes the device operable

// BLE
#define BLE_DEVICE_UUID "32890585-c6ee-498b-9e7a-044baefb6542"
#define BLE_COMMAND_UUID "c3fe2075-ac6c-40bf-8073-73a110453725"
#define BLE_CONF_UUID "c04a9b9c-3ab6-4cce-9b59-1b582112e693"
#define BLE_WIND_DATA_UUID "003d0cab-70f7-43ac-8ab9-db26466572af"
#define BLE_DEVICE_NAME "Wind"
#define BLE_COMMAND_CHARACTERISTIC_NAME "command"
#define BLE_DATA_CHARACTERISTIC_NAME "data"
#define BLE_CONF_CHARACTERISTIC_NAME "conf"

// The reading gives sin and cos, and sqrt(sin^2+cos^2 )must be equal to 1.0, or close to it. Large differences means that the linearization
// of the sin and cos sensors is off. This define the admissible delta, which essentially mesaure the eccentricity of measure.
#define ELLIPSE_VALIDITY_THRESHOLD 0.2

// admissible value ranges for configuration 
#define SMOOTHING_ALPHA_MIN 0
#define SMOOTHING_ALPHA_MAX 100

#define AUTO_CALIB_THRESHOLD_MIN 50
#define AUTO_CALIB_THRESHOLD_MAX 100

#endif