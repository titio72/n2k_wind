#ifndef _WIND_SPEED_H
#define _WIND_SPEED_H

struct wind_data;
class Conf;

class WindSpeed
{
public:
    WindSpeed();
    ~WindSpeed();

    unsigned long get_sample_age() { return last_read_time; }

    void setup();

    void read_data(wind_data &data, unsigned long milliseconds);

    void loop_micros(unsigned long now_micros);

    void set_speed_adjustment(double f);

    void apply_configuration(Conf& conf);

private:
    unsigned long last_read_time = 0;
    double adjustment_factor = 1.0;
    uint8_t vane_type;

    unsigned long counter = 0;
    int state = LOW;

    double smooth_counter = 0.0;
};

#endif