#include "WindUtil.h"
#include "Wind360.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

#pragma region Range
Range::Range() : l(RANGE_DEFAULT_MIN), h(RANGE_DEFAULT_MAX), minimum_valid_span(RANGE_DEFAULT_VALID) {}

Range::Range(uint16_t _low, uint16_t _high, uint16_t _valid_span) : l(_low), h(_high), minimum_valid_span(_valid_span) {}

bool Range::is_valid() const
{
    return range() > minimum_valid_span;
}

void Range::set(const Range& r)
{
     l = r.l;
     h = r.h;
     minimum_valid_span = r.minimum_valid_span;
}

void Range::set(uint16_t low, uint16_t high)
{
    l = low;
    h = high;
}

void Range::expand(uint16_t new_sample)
{
    l = l<new_sample ? l : new_sample;
    h = h>new_sample ? h : new_sample;
}

double Range::to_analog(double v_low, double v_high, uint16_t reading) const
{
    if (is_valid())
    {
        double d_r = reading;
        double d_l = low();
        double d_s = range();
        return (d_r - d_l) * (v_high - v_low) / d_s + v_low;
    }
    else
    {
        return NAN;
    }
}
#pragma endregion

void set_error(uint8_t& error, bool condition, uint8_t error_flag)
{
    if (condition)
    {
        error |= error_flag;
    }
    else
    {
        error &= ~error_flag;
    }
}

double norm_deg(double d)
{
    d = d - (int)(d / 360) * 360;
    if (d < 0)
        return d + 360.0;
    else
        return d;
}

int16_t norm_deg(int16_t d)
{
    d = d % 360;
    if (d < 0)
        return d + 360;
    else
        return d;
}

char *mystrtok(char **m, char *s, char c)
{
    char *p = s ? s : *m;
    if (!*p)
        return 0;
    *m = strchr(p, c);
    if (*m)
        *(*m)++ = 0;
    else
        *m = p + strlen(p);
    return p;
}

bool atoi_x(int32_t &value, const char *s_value)
{
    int v = 0;
    if (s_value && s_value[0] != '\0')
    {
        int p = 1;
        for (int i = strlen(s_value) - 1; i >= 0; i--)
        {
            if (s_value[i] == '-' && i == 0)
            {
                v = -v;
            }
            else
            {
                if (s_value[i] < '0' || s_value[i] > '9')
                    return false;
                v += p * (s_value[i] - '0');
                p *= 10;
            }
        }
        value = v;
        return true;
    }
    else
    {
        return false;
    }
}

bool parse_value(int32_t &target_value, const char *s_value, int32_t max_value, int32_t min_value)
{
    int32_t value = -1;
    if (s_value && atoi_x(value, s_value) && value >= min_value && value <= max_value)
    {
        target_value = value;
        return true;
    }
    return false;
}

double lpf_angle(double previous, double current, double alpha)
{
    if (isnan(previous)) return current;
    if (isnan(current)) return previous;
    double diff = current - previous;
    if (diff > 180.0) diff -= 360.0; else if (diff < -180.0) diff += 360.0;
   return norm_deg(previous + alpha * diff);
}
