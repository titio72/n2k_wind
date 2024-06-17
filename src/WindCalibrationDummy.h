#ifndef _WIND_CALIBRATION_DUMMY_H
#define _WIND_CALIBRAITON_DUMMY_H
#include <math.h>
#include <stdint.h>
#include "WindUtil.h"


class WindCalibrationDummy
{
public:
    WindCalibrationDummy(int size);
    ~WindCalibrationDummy();

    bool add_sample(int16_t sample);

    bool calibrate();

    void reset();

    Range get_calibrated() { return calibrated; }

private:
    Range calibrated;
    int size;

};
#endif
