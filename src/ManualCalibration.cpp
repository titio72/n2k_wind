#include "ManualCalibration.h"
#include <Log.h>

ManualCalibration::ManualCalibration(void (*on_complete)(Range &s_range, Range &c_range)): sin_calibration(MAX_ADC_RANGE), cos_calibration(MAX_ADC_RANGE)
{
  on_manual_calibration_complete = on_complete;
}

ManualCalibration::~ManualCalibration()
{
}

void ManualCalibration::record_reading(uint16_t sin_reading, uint16_t cos_reading, double angle)
{
    if (!in_progress) return;
    
    w360.set_degree(angle);
    sin_calibration.add_sample(sin_reading);
    cos_calibration.add_sample(cos_reading);
}

void ManualCalibration::start()
{
    in_progress = true;
    sin_calibration.reset();
    cos_calibration.reset();
    w360.reset();
}

void ManualCalibration::abort()
{
    in_progress = false;
}

void ManualCalibration::finalize()
{
  // finalize calibration
  if (w360.is_valid())
  {
    Log::trace("[CAL] Complete calibration\n");
    in_progress = false;
    bool cal_sin_ok = sin_calibration.calibrate();
    bool cal_cos_ok = sin_calibration.calibrate();
    if (cal_sin_ok && cal_cos_ok)
    {
      if (on_manual_calibration_complete) 
      {
        Range s_range = sin_calibration.get_calibrated();
        Range c_range = cos_calibration.get_calibrated();
        on_manual_calibration_complete(s_range, c_range);
      }
    }
    else
    {
      Log::trace("[CAL] Invalid ranges to complete calibration\n");
    }
  }
  else
  {
    Log::trace("[CAL] Not enough data to complete calibration\n");
    in_progress = false;
  }
}   