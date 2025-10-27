#ifndef _WIND_UTIL_H
#define _WIND_UTIL_H
#include <stdint.h>

#define WIND_ERROR_NO_CAL_OR_SIGNAL 1
#define WIND_ERROR_OK 0

#define to_radians(angleInDegrees) ((angleInDegrees) * M_PI / 180.0)
#define to_degrees(angleInRadians) ((angleInRadians) * 180.0 / M_PI)
#define max(v1, v2) ((v1 < v2) ? v2 : v1)
#define min(v1, v2) ((v1 < v2) ? v1 : v2)

struct wind_data
{
    // angle data
    double angle = 0.0;
    double ellipse = 1.0;
    int error = WIND_ERROR_NO_CAL_OR_SIGNAL;
    uint16_t i_sin = 0;
    uint16_t i_cos = 0;
    uint16_t offset = 0;

    // speed data
    double speed = 0.0;
    double frequency = 0.0;
    int error_speed = WIND_ERROR_NO_CAL_OR_SIGNAL;

    double angle_smoothing_factor = 1.0;
    double speed_smoothing_factor = 1.0;
};

class Range
{
public:
    Range();
    Range(uint16_t low, uint16_t high, uint16_t valid_span);

    uint16_t low();
    uint16_t high();
    uint16_t mid();
    uint16_t size();
    bool valid();

    void set(uint16_t low, uint16_t high);
    void set(const Range &range) { set(range.l, range.h); }

private:
    uint16_t l;
    uint16_t h;
    uint16_t valid_span;
};

double to_analog(uint16_t reading, double v_low, double v_high, Range Range);

uint16_t to_digital(double value, double v_low, double v_high, Range range);

double norm_deg(double d);

int16_t norm_deg(int16_t d);

double get_angle_deg(int16_t sin_reading, Range &sin_calibration, int16_t cos_reading, Range &cos_calibration);

int16_t get_noise(int16_t amplitude);

char *mystrtok(char **m, char *s, char c);

bool atoi_x(int32_t &value, const char *s_value);

bool parse_value(int32_t &target_value, const char *s_value, uint16_t max_value);

void addInt(uint8_t *dest, int &offset, uint32_t data32);

void addShort(uint8_t *dest, int &offset, uint16_t data16);

void addChar(uint8_t *dest, int &offset, uint8_t data8);

#endif
