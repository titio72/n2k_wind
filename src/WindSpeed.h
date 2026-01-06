#ifndef _WIND_SPEED_H
#define _WIND_SPEED_H

struct wind_data;
struct configuration;

#include <SpeedSensor.h>
#include <SpeedSensorInterrupt.h>

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
    SpeedSensor speed_sensor;
    SpeedSensorInterrupt speed_sensor_interrupt;

    double adjustment_factor = 1.0;
    uint8_t vane_type;
};

#endif