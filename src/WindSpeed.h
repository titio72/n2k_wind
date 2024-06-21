#ifndef _WIND_SPEED_H
#define _WIND_SPEED_H

#define STATE_UP 1
#define STATE_DOWN -1
#define STATE_UNKNOWN 0

class WindSpeed
{
public:
    WindSpeed(bool simulate, uint32_t tick_microseconds);
    ~WindSpeed();

    void get_speed(double &speed, double &frequency, int &error);

    void setup();

    void loop_micros();

private:
    bool simulate;
    double frequency;
    volatile int state;
    volatile uint64_t counter;
    volatile uint64_t period_micros;
    uint16_t *buffer;
    uint32_t buffer_ix;
    uint16_t tick_micros;
};

#endif