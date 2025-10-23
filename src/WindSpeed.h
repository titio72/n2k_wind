#ifndef _WIND_SPEED_H
#define _WIND_SPEED_H

#define STATE_UP 1
#define STATE_DOWN -1
#define STATE_UNKNOWN 0

class WindSpeed
{
public:
    WindSpeed(bool simulate);
    ~WindSpeed();

    void get_speed(double &speed, double &frequency, int &error, unsigned long t);

    void setup();

    void loop_micros(unsigned long now_micros);

    void set_apparent_wind_angle(double deg);

    void set_speed_adjustment(double f);

    unsigned long counter = 0;

private:
    bool simulate;
    double frequency;
    int state;
    double hz_to_knots;

    // used for simulation
    double apparent_wind_angle;
    unsigned long sim_period;

    unsigned long last_read_time = 0;

    unsigned long last_period = 0;
    unsigned long last_state_change_time = 0;

};

#endif