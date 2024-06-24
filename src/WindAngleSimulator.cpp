#include "WindAngleSImulator.h"
#include <math.h>

#define SIMUL_RANGE_SIN_LOW 1024
#define SIMUL_RANGE_SIN_HIGH 3072
#define SIMUL_RANGE_COS_LOW 1024
#define SIMUL_RANGE_COS_HIGH 3072
#define SIMUL_DEG_PER_SECOND 6.0

WindAngleSimulator::WindAngleSimulator():
    sim_sin(SIMUL_RANGE_SIN_LOW, SIMUL_RANGE_SIN_HIGH, 0), sim_cos(SIMUL_RANGE_COS_LOW, SIMUL_RANGE_COS_HIGH, 0), sim_deg(0.0), sim_t0(0)
{}

double WindAngleSimulator::get_sim_angle(unsigned long now_micros)
{
    unsigned long dt = now_micros - sim_t0;
    sim_t0 = now_micros;
    sim_deg = norm_deg(sim_deg + (SIMUL_DEG_PER_SECOND* (dt / 1000000.0)));
    return sim_deg;
}

void WindAngleSimulator::sim_values(uint16_t &s, uint16_t &c, unsigned long now_micros, double &expected)
{
    double deg = get_sim_angle(now_micros);
    double v_sin = sin(to_radians(deg));
    double v_cos = cos(to_radians(deg));
    s = to_digital(v_sin, -1, 1, sim_sin) + get_noise(sim_sin.size() * 0.05);
    c = to_digital(v_cos, -1, 1, sim_cos) + get_noise(sim_cos.size() * 0.05);
    expected = deg;
}