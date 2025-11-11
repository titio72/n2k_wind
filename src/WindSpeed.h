#ifndef _WIND_SPEED_H
#define _WIND_SPEED_H

#define SMOOTHING_BUFFER_SIZE 32

struct wind_data;

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

private:
    unsigned long last_read_time = 0;
    double hz_to_knots;

    unsigned long counter = 0;
    int state = LOW;
    unsigned long last_state_change_time = 0;

    unsigned long period = 0;
    double smooth_period = 0.0;
    double smooth_counter = 0.0;
};

#endif