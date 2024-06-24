#ifndef _WIND_ANGLE_SIMULATOR_H
#define _WIND_ANGLE_SIMULATOR_H
#include <stdlib.h>
#include <WindUtil.h>

class WindAngleSimulator
{
public:
    WindAngleSimulator();

    void sim_values(uint16_t &s, uint16_t &c, unsigned long now_micros, double &expected);

private:
    double get_sim_angle(unsigned long now_micros);
    Range sim_sin;
    Range sim_cos;
    double sim_deg;
    unsigned long sim_t0;
};

#endif
