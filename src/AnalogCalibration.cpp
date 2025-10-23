#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "WindUtil.h"
#include "AnalogCalibration.h"

AnalogCalibration::AnalogCalibration(int s): calibrated(s, 0, 1), size(s)
{
}

AnalogCalibration::~AnalogCalibration()
{
}

bool AnalogCalibration::add_sample(int16_t reading)
{
    if (reading>=0 && reading<size)
    {
        if (reading<calibrated.low()) calibrated.set(reading, calibrated.high());
        else if (reading>calibrated.high()) calibrated.set(calibrated.low(), reading);
        return true;
    }
    else
    {
        return false;
    }
}

bool AnalogCalibration::calibrate()
{
    return calibrated.valid();
}

void AnalogCalibration::reset()
{
    calibrated.set(size, 0);
}