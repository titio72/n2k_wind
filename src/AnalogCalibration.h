#ifndef _ANALOG_CALIBRATION_H
#define _ANALOG_CALIBRATION_H

#include "WindUtil.h"


class AnalogCalibration
{
public:
    AnalogCalibration(int size);
    ~AnalogCalibration();

    bool add_sample(int16_t sample);

    bool calibrate();

    void reset();

    Range get_calibrated() { return calibrated; }

private:
    Range calibrated;
    int size;
};
#endif
