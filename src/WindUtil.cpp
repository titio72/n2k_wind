#include "WindUtil.h"
#include "Wind360.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

#pragma region Range
Range::Range() : l(RANGE_DEFAULT_MIN), h(RANGE_DEFAULT_MAX), minimum_valid_span(RANGE_DEFAULT_VALID) {} // init invalid

Range::Range(uint16_t _low, uint16_t _high, uint16_t _valid_span) : l(_low), h(_high), minimum_valid_span(_valid_span) {}

bool Range::is_valid()
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

double Range::to_analog(double v_low, double v_high, uint16_t reading)
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

double to_analog(uint16_t reading, double v_low, double v_high, Range range)
{
    return range.to_analog(v_low, v_high, reading);
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

bool parse_value(int32_t &target_value, const char *s_value, int32_t max_value)
{
    int32_t value = -1;
    if (s_value && atoi_x(value, s_value) && value > 0 && value <= max_value)
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

ByteBuffer::ByteBuffer(size_t size) : buf_size(size), offset(0) {
    buffer = new uint8_t[size];
}

ByteBuffer::~ByteBuffer()
{
    delete[] buffer;
}

ByteBuffer& ByteBuffer::operator<< (Wind360 &w)
{
    *this << (uint8_t)w.size();
    for (int i = 0; i < w.buffer_size(); i++) *this << (uint8_t)w.get_data(i);
    *this << (uint8_t)round(w.get_score() * 100.0);
    return *this;
}

ByteBuffer &ByteBuffer::operator<< (const char* t)
{
    size_t t_size = strlen(t);
    if (t_size<255 && offset + t_size <= buf_size) {
        *this << (uint8_t)t_size;
        memcpy(buffer + offset, t, t_size);
        offset += t_size;
    }
    return *this;
}

void ByteBuffer::reset()
{
    offset = 0;
}