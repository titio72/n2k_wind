#ifndef _WIND_UTIL_H
#define _WIND_UTIL_H
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <Utils.h>
#include "Constants.h"

#define to_radians(angleInDegrees) ((angleInDegrees) * M_PI / 180.0)
#define to_degrees(angleInRadians) ((angleInRadians) * 180.0 / M_PI)

class Wind360;

class Range
{
public:
    Range();
    Range(uint16_t low, uint16_t high, uint16_t valid_span = RANGE_DEFAULT_VALID);

    uint16_t low() const { return l; }
    uint16_t high() const { return h; }
    uint16_t range() const { return (h > l) ? (h - l) : 0; }
    bool is_valid() const;

    void set(uint16_t low, uint16_t high);
    void set(const Range &range);

    Range &operator=(const Range &range)
    {
        set(range);
        return *this;
    }

    void expand(uint16_t new_value);

    double to_analog(double min, double max, uint16_t value) const;

private:
    uint16_t l;
    uint16_t h;
    uint16_t minimum_valid_span;
};

typedef bool (*calibration_callback)(const Range &s_range, const Range &c_range);

char *mystrtok(char **m, char *s, char c);

bool atoi_x(int32_t &value, const char *s_value);

bool parse_value(int32_t &target_value, const char *s_value, int32_t max_value, int32_t min_value = INT32_MIN);

double lpf_angle(double previous, double current, double alpha);

void set_error(uint8_t& error, bool condition, uint8_t error_flag);

#endif
