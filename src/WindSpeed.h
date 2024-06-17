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

    void get_speed(double &speed, double &frequency, int &error);

    void setup();

    void loop_micros();

private:
    bool simulate;
    double frequency;
    int state;
    uint64_t state_time;
    uint64_t counter;
    uint64_t last_counter;
    uint16_t *buffer;
    uint32_t buffer_ix;
};

#endif