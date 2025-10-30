#include "WindUtil.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

#pragma region Range
Range::Range() : l(UINT16_MAX), h(0), valid_span(1) {} // init invalid

Range::Range(uint16_t _low, uint16_t _high, uint16_t _valid_span) : l(_low), h(_high), valid_span(_valid_span) {}

uint16_t Range::low()
{
    return l;
}

uint16_t Range::high()
{
    return h;
}

uint16_t Range::mid()
{
    return (int16_t)((h + l) / 2.0 + 0.5);
}

uint16_t Range::size()
{
    return h - l + 1;
}

bool Range::valid()
{
    return size() > valid_span;
}

void Range::set(uint16_t low, uint16_t high)
{
    l = low;
    h = high;
}
#pragma endregion

double to_analog(uint16_t reading, double v_low, double v_high, Range range)
{
    if (range.valid())
    {
        double d_r = reading;
        double d_l = range.low();
        double d_s = range.size();
        return (d_r - d_l) * (v_high - v_low) / d_s + v_low;
    }
    else
    {
        return NAN;
    }
}

uint16_t to_digital(double value, double v_low, double v_high, Range range)
{
    if (range.valid() && (v_low < v_high))
    {
        return (uint16_t)((value - v_low) / (v_high - v_low) * range.size() + range.low() + 0.5);
    }
    else
    {
        return UINT16_MAX;
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
    if (d < 0)
        return (d % 360) + 360;
    else
        return d % 360;
}

double get_angle_deg(uint16_t sin_reading, Range &sin_calibration, int16_t cos_reading, Range &cos_calibration)
{
    double v_sin = to_analog(sin_reading, -1, 1, sin_calibration);
    double v_cos = to_analog(cos_reading, -1, 1, cos_calibration);
    return norm_deg(to_radians(atan2(v_sin, v_cos)));
}

int16_t get_noise(int16_t amplitude)
{
    double r = ((double)rand() / (double)RAND_MAX);
    double v = (r * amplitude - 0.5 * amplitude);
    return (int16_t)(v + 0.5);
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
    if (s_value)
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

void addInt(uint8_t *dest, int &offset, uint32_t data32)
{
    uint8_t *temp = dest + offset;
    temp[0] = data32;
    temp[1] = data32 >> 8;
    temp[2] = data32 >> 16;
    temp[3] = data32 >> 24;
    offset += 4;
}

void addShort(uint8_t *dest, int &offset, uint16_t data16)
{
    uint8_t *temp = dest + offset;
    temp[0] = data16;
    temp[1] = data16 >> 8;
    offset += 2;
}

void addChar(uint8_t *dest, int &offset, uint8_t data8)
{
    uint8_t *temp = dest + offset;
    temp[0] = data8;
    offset += 1;
}

bool parse_value(int32_t &target_value, const char *s_value, uint16_t max_value)
{
    int32_t value = -1;
    if (s_value && atoi_x(value, s_value) && value > 0 && value <= max_value)
    {
        target_value = value;
        return true;
    }
    return false;
}

/*
int main(int arc, const char** argv) {
    const char* _c = "123|234|345|456";
    char* c = strdup(_c);

    int32_t vv[] = {0,0,0,0};

    char *t;
    char* p = c;
    int i_tok = 0;
    for (t = mystrtok(&p, c, '|'); t && i_tok<4; t = mystrtok(&p, 0, '|'))
    {
        if (strlen(t))
        {
            parse_value(vv[i_tok], t, 4095);
        }
        i_tok++;
    }
    printf("%d %d %d %d\n", vv[0], vv[1], vv[2], vv[3]);

   return 0;
}
*/