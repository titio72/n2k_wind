#include "BLESerializer.h"
#include "DataAndConf.h"
#include "Calibration.h"
#include <Utils.h>
#include <Log.h>

template<>
ByteBuffer& ByteBuffer::operator<<(const Wind360 &w)
{
    *this << (uint8_t)w.size();
    for (int i = 0; i < w.buffer_size(); i++) *this << (uint8_t)w.get_data(i);
    *this << (uint8_t)round(w.get_score() * 100.0);
    return *this;
}

template<>
ByteBuffer& ByteBuffer::operator<<(const Range &r)
{
    *this << r.low() << r.high();
    return *this;
}

ByteBuffer& make_message(const wind_data& wdata, const Calibration &calib, ByteBuffer& buffer)
{    
    uint32_t mem = wdata.heap;
    uint16_t i_angle = ((int16_t)(wdata.angle * 10 + 0.5) + 3600) % 3600;  
    uint16_t i_smooth_angle = ((int16_t)(wdata.smooth_angle * 10 + 0.5) + 3600) % 3600;
    uint16_t i_output_angle = ((i_smooth_angle + wdata.conf.offset * 10) + 3600) % 3600;
    uint16_t i_ellipse = (uint16_t)round(wdata.ellipse * 1000);
    uint16_t i_speed = isnan(wdata.speed) ? 0 : (uint16_t)(wdata.speed * 10 + 0.5);

    buffer.reset();
    buffer << i_angle                   // 0-1
            << i_smooth_angle           // 2-3  
            << i_output_angle           // 4-5
            << i_ellipse                // 6-7
            << mem                      // 8-11
            << wdata.angle_error        // 12
            << wdata.i_sin              // 13-14
            << wdata.conf.sin_range     // 15-18
            << wdata.i_cos              // 19-20
            << wdata.conf.cos_range     // 21-24
            << i_speed                  // 25-26
            << wdata.speed_error        // 27
            << wdata.conf.offset        // 28-29
            << wdata.conf.speed_adjustment              // 30
            << wdata.conf.n2k_source                    // 31
            << wdata.conf.angle_smoothing               // 32
            << wdata.conf.speed_smoothing               // 33
            << wdata.conf.calibration_score_threshold   // 34
            << wdata.conf.auto_cal                      // 35
            << wdata.n2k_err                            // 36
            << wdata.conf.vane_type                     // 37
            << (calib.is_calibration_valid()?YES:NO)    // 38
            << (calib.is_calibration_in_score()?YES:NO) // 39
            << (Log::is_enabled()?YES:NO)               // 40
            << calib.get_wind360();                     // 41-54 The variable length part at the end

    return buffer;
}