#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "WindUtil.h"
#include "WindCalibrationDummy.h"

WindCalibrationDummy::WindCalibrationDummy(int s): calibrated(s, 0, 1), size(s)
{
}

WindCalibrationDummy::~WindCalibrationDummy()
{
}

bool WindCalibrationDummy::add_sample(int16_t reading)
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

bool WindCalibrationDummy::calibrate()
{
    return calibrated.valid();
}

void WindCalibrationDummy::reset()
{
    calibrated.set(size, 0);
}