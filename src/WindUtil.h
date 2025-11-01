#ifndef _WIND_UTIL_H
#define _WIND_UTIL_H
#include <stdint.h>
#include <string.h>
#include <math.h>

// 12 bit ADC for ESP32
#define MAX_ADC_VALUE 4095
#define MAX_ADC_RANGE 4096
#define RANGE_DEFAULT_MIN 1024
#define RANGE_DEFAULT_MAX 3072
#define RANGE_DEFAULT_VALID 512

#define WIND_ERROR_NO_CAL_OR_SIGNAL 1
#define WIND_ERROR_OK 0

#define to_radians(angleInDegrees) ((angleInDegrees) * M_PI / 180.0)
#define to_degrees(angleInRadians) ((angleInRadians) * 180.0 / M_PI)

struct wind_data
{
    // angle data
    double angle = 0.0;
    double smooth_angle = 0.0;
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

    uint16_t low() { return l; }
    uint16_t high() { return h; }
    uint16_t mid() { return (uint16_t)round((l+h)/2); }
    uint16_t size() { return h - l; }
    bool valid();

    void set(uint16_t low, uint16_t high);
    void set(const Range &range) { set(range.l, range.h); }

    Range &operator=(const Range &range)
    {
        set(range);
        return *this;
    }

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

double lpf_angle(double previous, double current, double alpha);

class ByteBuffer
{
public:
    ByteBuffer(size_t size) : buf_size(size), offset(0) {
        buffer = new uint8_t[size];
    }

    ~ByteBuffer()
    {
        delete[] buffer;
    }

    ByteBuffer &operator<< (const char* t)
    {
        size_t t_size = strlen(t);
        if (offset + t_size <= buf_size) {
            strcpy((char*)(buffer + offset), t);
            offset += t_size;
        }
        return *this;
    }

    template<typename T>
    ByteBuffer &operator<< (T t)
    {
        size_t t_size = sizeof(T);
        if (offset + t_size <= buf_size) {
            *((T*)(buffer + offset)) = t;
            offset += t_size;
        }
        return *this;
    }

    void reset()
    {
        offset = 0;
    }

    uint8_t* data() { return buffer; }
    size_t size() { return buf_size; }
    size_t length() { return offset; }

private:
    uint8_t *buffer;
    size_t buf_size;
    size_t offset;
};

#endif
