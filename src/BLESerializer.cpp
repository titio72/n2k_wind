#include "BLESerializer.h"
#include "DataAndConf.h"
#include "Calibration.h"
#include "Constants.h"
#include "CorrectionProtocol.h"
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

ByteBuffer& make_message(const configuration &conf, const all_data &wdata, const Calibration &calib, ByteBuffer &buffer)
{    
    uint32_t mem = wdata.heap;
    uint16_t i_angle = ((int16_t)(wdata.wind.angle * 10 + 0.5) + 3600) % 3600;  
    uint16_t i_smooth_angle = ((int16_t)(wdata.wind.smooth_angle * 10 + 0.5) + 3600) % 3600;
    uint16_t i_output_angle = ((i_smooth_angle + conf.offset * 10) + 3600) % 3600;
    uint16_t i_ellipse = (uint16_t)round(wdata.wind.ellipse * 1000);
    uint16_t i_speed = isnan(wdata.wind.speed) ? 0 : (uint16_t)(wdata.wind.speed * 10 + 0.5);
    uint16_t i_frequency = isnan(wdata.wind.frequency) ? 0 : (uint16_t)(wdata.wind.frequency * 100 + 0.5);

    uint16_t i_corr_awa = isnan(wdata.correction.awa) ? 0xFFFF : (uint16_t)(((int32_t)(wdata.correction.awa * 10 + 0.5) % 3600 + 3600) % 3600);
    uint16_t i_corr_aws = isnan(wdata.correction.aws) ? 0xFFFF : (uint16_t)(wdata.correction.aws * 10 + 0.5);

    buffer.reset();
    buffer << i_angle                   // 0
            << i_smooth_angle           // 2  
            << i_output_angle           // 4
            << i_ellipse                // 6
            << mem                      // 8
            << wdata.wind.angle_error   // 12
            << wdata.wind.i_sin         // 13
            << conf.sin_range           // 15
            << wdata.wind.i_cos         // 19
            << conf.cos_range           // 21
            << i_speed                  // 25
            << i_frequency              // 27
            << wdata.wind.speed_error   // 29
            << conf.offset              // 30
            << conf.speed_adjustment    // 32
            << conf.n2k_source          // 33
            << conf.angle_smoothing     // 34
            << conf.speed_smoothing     // 35
            << conf.calibration_score_threshold   // 36
            << conf.auto_cal            // 37
            << wdata.n2k_err            // 38
            << conf.vane_type           // 39
            << (calib.is_calibration_valid()?YES:NO)    // 40
            << (calib.is_calibration_in_score()?YES:NO) // 41
            << (Log::is_enabled()?YES:NO)               // 42
            << calib.get_wind360()                      // 43-56 The variable length part
            // correction status, after the variable part (offset 43 + 1 + wind360 buffer + 1)
            << wdata.correction.table_id                // +0 u32
            << wdata.correction.enabled                 // +4 u8
            << wdata.correction.corrected               // +5 u8
            << i_corr_awa                               // +6 u16 0.1 deg, 0xFFFF = n/a
            << i_corr_aws;                              // +8 u16 0.1 kn, 0xFFFF = n/a

    return buffer;
}

size_t make_table_message(const correction_table &t, uint8_t *buf, size_t buf_len)
{
    return encode_correction_table(t, N2K_MANIFACTURER_CODE, buf, buf_len);
}
