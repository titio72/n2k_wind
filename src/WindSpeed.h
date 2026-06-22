#ifndef _WIND_SPEED_H
#define _WIND_SPEED_H

#define USE_SPEED_SENSOR_INTERRUPT 1 // set to true to use SpeedSensorInterrupt instead of SpeedSensor


struct wind_data;
struct configuration;

#if USE_SPEED_SENSOR_INTERRUPT==1
#include <SpeedSensorInterrupt.h>
#else
#include <SpeedSensor.h>
#endif

class WindSpeed
{
public:
    WindSpeed(int pin);
    ~WindSpeed();

    unsigned long get_sample_age() const { return speed_sensor.get_sample_age(); }

    void setup();

    void read_data(wind_data &data, configuration & conf, unsigned long milliseconds);

    void loop_micros(unsigned long now_micros);

    void apply_configuration(configuration& conf);

private:
    #if USE_SPEED_SENSOR_INTERRUPT==1
    SpeedSensorInterrupt speed_sensor;
    #else
    SpeedSensor speed_sensor;
    #endif

    double adjustment_factor = 1.0;
    uint8_t vane_type;

    unsigned long last_valid_read_ts = 0;
};

#endif