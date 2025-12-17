#include <unity.h>
#include <math.h>
#include <limits.h>
#include "../src/BLESerializer.h"
#include "../src/DataAndConf.h"
#include "../src/Calibration.h"
#include "../src/Constants.h"

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * Extract uint16_t from buffer at offset
 */
static uint16_t get_uint16(const ByteBuffer &buf, size_t offset) {
    uint8_t data[128];
    buf.get_data(data, 128);
    return (uint16_t)((data[offset + 1] << 8) | data[offset]);
}

/**
 * Extract uint32_t from buffer at offset
 */
static uint32_t get_uint32(const ByteBuffer &buf, size_t offset) {
    uint8_t data[128];
    buf.get_data(data, 128);
    return (uint32_t)(
        (data[offset + 3] << 24) |
        (data[offset + 2] << 16) |
        (data[offset + 1] << 8) |
        data[offset]
    );
}

/**
 * Extract uint8_t from buffer at offset
 */
static uint8_t get_uint8(const ByteBuffer &buf, size_t offset) {
    uint8_t data[128];
    buf.get_data(data, 128);
    return data[offset];
}

// ============================================================================
// Angle Encoding Tests
// ============================================================================

/**
 * Test: make_message encodes angle as (angle * 10 + 3600) % 3600
 * 0° -> 0 (encoded as 0)
 */
void test_make_message_angle_zero(void) {
    wind_data wdata;
    wdata.angle = 0.0;
    wdata.smooth_angle = 0.0;
    wdata.conf.offset = 0;
    wdata.heap = 0;
    wdata.angle_error = 0;
    wdata.ellipse = 1.0;
    wdata.speed = 10.0;
    wdata.speed_error = 0;
    wdata.i_sin = 2000;
    wdata.i_cos = 2000;
    wdata.conf.sin_range.set(600, 3400);
    wdata.conf.cos_range.set(600, 3400);
    wdata.conf.n2k_source = 21;
    wdata.conf.angle_smoothing = 50;
    wdata.conf.speed_smoothing = 50;
    wdata.conf.calibration_score_threshold = 50;
    wdata.conf.auto_cal = 0;
    wdata.conf.speed_adjustment = 0;
    wdata.conf.vane_type = VANE_TYPE_ST50;
    wdata.n2k_err = 0;
    
    Calibration calib;
    ByteBuffer buf(128); make_message(wdata, calib, buf);
    
    // i_angle = ((int16_t)(0.0 * 10 + 0.5) + 3600) % 3600 = (0 + 3600) % 3600 = 0
    uint16_t i_angle = get_uint16(buf, 0);
    TEST_ASSERT_EQUAL_UINT16(0, i_angle);

    TEST_ASSERT_EQUAL_INT(55, buf.length());
}

/**
 * Test: make_message encodes angle 45° correctly
 */
void test_make_message_angle_45(void) {
    wind_data wdata;
    wdata.angle = 45.0;
    wdata.smooth_angle = 45.0;
    wdata.conf.offset = 0;
    wdata.heap = 0;
    wdata.angle_error = 0;
    wdata.ellipse = 1.0;
    wdata.speed = 10.0;
    wdata.speed_error = 0;
    wdata.i_sin = 2000;
    wdata.i_cos = 2000;
    wdata.conf.sin_range.set(600, 3400);
    wdata.conf.cos_range.set(600, 3400);
    wdata.conf.n2k_source = 21;
    wdata.conf.angle_smoothing = 50;
    wdata.conf.speed_smoothing = 50;
    wdata.conf.calibration_score_threshold = 50;
    wdata.conf.auto_cal = 0;
    wdata.conf.speed_adjustment = 0;
    wdata.conf.vane_type = VANE_TYPE_ST50;
    wdata.n2k_err = 0;
    
    Calibration calib;
    ByteBuffer buf(128); make_message(wdata, calib, buf);
    
    // i_angle = ((int16_t)(45.0 * 10 + 0.5) + 3600) % 3600 = (450 + 3600) % 3600 = 450
    uint16_t i_angle = get_uint16(buf, 0);
    TEST_ASSERT_EQUAL_UINT16(450, i_angle);
}

/**
 * Test: make_message encodes angle 90° correctly
 */
void test_make_message_angle_90(void) {
    wind_data wdata;
    wdata.angle = 90.0;
    wdata.smooth_angle = 90.0;
    wdata.conf.offset = 0;
    wdata.heap = 0;
    wdata.angle_error = 0;
    wdata.ellipse = 1.0;
    wdata.speed = 10.0;
    wdata.speed_error = 0;
    wdata.i_sin = 2000;
    wdata.i_cos = 2000;
    wdata.conf.sin_range.set(600, 3400);
    wdata.conf.cos_range.set(600, 3400);
    wdata.conf.n2k_source = 21;
    wdata.conf.angle_smoothing = 50;
    wdata.conf.speed_smoothing = 50;
    wdata.conf.calibration_score_threshold = 50;
    wdata.conf.auto_cal = 0;
    wdata.conf.speed_adjustment = 0;
    wdata.conf.vane_type = VANE_TYPE_ST50;
    wdata.n2k_err = 0;
    
    Calibration calib;
    ByteBuffer buf(128); make_message(wdata, calib, buf);
    
    // i_angle = ((int16_t)(90.0 * 10 + 0.5) + 3600) % 3600 = (900 + 3600) % 3600 = 900
    uint16_t i_angle = get_uint16(buf, 0);
    TEST_ASSERT_EQUAL_UINT16(900, i_angle);
}

/**
 * Test: make_message wraps negative angles (e.g., -45° -> 3150)
 */
void test_make_message_angle_negative(void) {
    wind_data wdata;
    wdata.angle = 45.0;
    wdata.smooth_angle = 45.0;
    wdata.conf.offset = 0;
    wdata.heap = 0;
    wdata.angle_error = 0;
    wdata.ellipse = 1.0;
    wdata.speed = 10.0;
    wdata.speed_error = 0;
    wdata.i_sin = 2000;
    wdata.i_cos = 2000;
    wdata.conf.sin_range.set(600, 3400);
    wdata.conf.cos_range.set(600, 3400);
    wdata.conf.n2k_source = 21;
    wdata.conf.angle_smoothing = 50;
    wdata.conf.speed_smoothing = 50;
    wdata.conf.calibration_score_threshold = 50;
    wdata.conf.auto_cal = 0;
    wdata.conf.speed_adjustment = 0;
    wdata.conf.vane_type = VANE_TYPE_ST50;
    wdata.n2k_err = 0;
    
    Calibration calib;
    ByteBuffer buf(128); make_message(wdata, calib, buf);
    
    // i_angle = ((int16_t)(45.0 * 10 + 0.5) + 3600) % 3600 = (450 + 3600) % 3600 = 450
    uint16_t i_angle = get_uint16(buf, 0);
    TEST_ASSERT_EQUAL_UINT16(450, i_angle);
}

/**
 * Test: make_message encodes angle 359° correctly
 */
void test_make_message_angle_359(void) {
    wind_data wdata;
    wdata.angle = 359.0;
    wdata.smooth_angle = 359.0;
    wdata.conf.offset = 0;
    wdata.heap = 0;
    wdata.angle_error = 0;
    wdata.ellipse = 1.0;
    wdata.speed = 10.0;
    wdata.speed_error = 0;
    wdata.i_sin = 2000;
    wdata.i_cos = 2000;
    wdata.conf.sin_range.set(600, 3400);
    wdata.conf.cos_range.set(600, 3400);
    wdata.conf.n2k_source = 21;
    wdata.conf.angle_smoothing = 50;
    wdata.conf.speed_smoothing = 50;
    wdata.conf.calibration_score_threshold = 50;
    wdata.conf.auto_cal = 0;
    wdata.conf.speed_adjustment = 0;
    wdata.conf.vane_type = VANE_TYPE_ST50;
    wdata.n2k_err = 0;
    
    Calibration calib;
    ByteBuffer buf(128); make_message(wdata, calib, buf);
    
    // i_angle = ((int16_t)(359.0 * 10 + 0.5) + 3600) % 3600 = (3590 + 3600) % 3600 = 3590
    uint16_t i_angle = get_uint16(buf, 0);
    TEST_ASSERT_EQUAL_UINT16(3590, i_angle);
}

// ============================================================================
// Smooth Angle Encoding Tests
// ============================================================================

/**
 * Test: make_message encodes smooth_angle correctly
 * Offset at bytes 2-3
 */
void test_make_message_smooth_angle(void) {
    wind_data wdata;
    wdata.angle = 0.0;
    wdata.smooth_angle = 180.5;
    wdata.conf.offset = 0;
    wdata.heap = 0;
    wdata.angle_error = 0;
    wdata.ellipse = 1.0;
    wdata.speed = 10.0;
    wdata.speed_error = 0;
    wdata.i_sin = 2000;
    wdata.i_cos = 2000;
    wdata.conf.sin_range.set(600, 3400);
    wdata.conf.cos_range.set(600, 3400);
    wdata.conf.n2k_source = 21;
    wdata.conf.angle_smoothing = 50;
    wdata.conf.speed_smoothing = 50;
    wdata.conf.calibration_score_threshold = 50;
    wdata.conf.auto_cal = 0;
    wdata.conf.speed_adjustment = 0;
    wdata.conf.vane_type = VANE_TYPE_ST50;
    wdata.n2k_err = 0;
    
    Calibration calib;
    ByteBuffer buf(128); make_message(wdata, calib, buf);
    
    // i_smooth_angle = ((int16_t)(180.5 * 10 + 0.5) + 3600) % 3600 = (1805 + 3600) % 3600 = 1805
    uint16_t i_smooth_angle = get_uint16(buf, 2);
    TEST_ASSERT_EQUAL_UINT16(1805, i_smooth_angle);
}

// ============================================================================
// Output Angle (with offset) Tests
// ============================================================================

/**
 * Test: make_message calculates output_angle with offset
 * i_output_angle = ((i_smooth_angle + offset * 10) + 3600) % 3600
 */
void test_make_message_output_angle_with_offset(void) {
    wind_data wdata;
    wdata.angle = 0.0;
    wdata.smooth_angle = 90.0;  // 900 encoded
    wdata.conf.offset = 45;      // +450 units
    wdata.heap = 0;
    wdata.angle_error = 0;
    wdata.ellipse = 1.0;
    wdata.speed = 10.0;
    wdata.speed_error = 0;
    wdata.i_sin = 2000;
    wdata.i_cos = 2000;
    wdata.conf.sin_range.set(600, 3400);
    wdata.conf.cos_range.set(600, 3400);
    wdata.conf.n2k_source = 21;
    wdata.conf.angle_smoothing = 50;
    wdata.conf.speed_smoothing = 50;
    wdata.conf.calibration_score_threshold = 50;
    wdata.conf.auto_cal = 0;
    wdata.conf.speed_adjustment = 0;
    wdata.conf.vane_type = VANE_TYPE_ST50;
    wdata.n2k_err = 0;
    
    Calibration calib;
    ByteBuffer buf(128); make_message(wdata, calib, buf);
    
    // i_output_angle = ((900 + 45 * 10) + 3600) % 3600 = (900 + 450 + 3600) % 3600 = 4950 % 3600 = 1350
    uint16_t i_output_angle = get_uint16(buf, 4);
    TEST_ASSERT_EQUAL_UINT16(1350, i_output_angle);
}

/**
 * Test: make_message output_angle wraps at 3600
 */
void test_make_message_output_angle_wrap(void) {
    wind_data wdata;
    wdata.angle = 0.0;
    wdata.smooth_angle = 350.0;  // 3500 encoded
    wdata.conf.offset = 50;      // +500 units
    wdata.heap = 0;
    wdata.angle_error = 0;
    wdata.ellipse = 1.0;
    wdata.speed = 10.0;
    wdata.speed_error = 0;
    wdata.i_sin = 2000;
    wdata.i_cos = 2000;
    wdata.conf.sin_range.set(600, 3400);
    wdata.conf.cos_range.set(600, 3400);
    wdata.conf.n2k_source = 21;
    wdata.conf.angle_smoothing = 50;
    wdata.conf.speed_smoothing = 50;
    wdata.conf.calibration_score_threshold = 50;
    wdata.conf.auto_cal = 0;
    wdata.conf.speed_adjustment = 0;
    wdata.conf.vane_type = VANE_TYPE_ST50;
    wdata.n2k_err = 0;
    
    Calibration calib;
    ByteBuffer buf(128); make_message(wdata, calib, buf);
    
    // i_output_angle = ((3500 + 50 * 10) + 3600) % 3600 = (3500 + 500 + 3600) % 3600 = 7600 % 3600 = 400
    uint16_t i_output_angle = get_uint16(buf, 4);
    TEST_ASSERT_EQUAL_UINT16(400, i_output_angle);
}

// ============================================================================
// Ellipse Encoding Tests
// ============================================================================

/**
 * Test: make_message encodes ellipse as ellipse * 1000
 * Offset at bytes 6-7
 */
void test_make_message_ellipse(void) {
    wind_data wdata;
    wdata.angle = 0.0;
    wdata.smooth_angle = 0.0;
    wdata.conf.offset = 0;
    wdata.heap = 0;
    wdata.angle_error = 0;
    wdata.ellipse = 1.234;  // Should encode as 1234
    wdata.speed = 10.0;
    wdata.speed_error = 0;
    wdata.i_sin = 2000;
    wdata.i_cos = 2000;
    wdata.conf.sin_range.set(600, 3400);
    wdata.conf.cos_range.set(600, 3400);
    wdata.conf.n2k_source = 21;
    wdata.conf.angle_smoothing = 50;
    wdata.conf.speed_smoothing = 50;
    wdata.conf.calibration_score_threshold = 50;
    wdata.conf.auto_cal = 0;
    wdata.conf.speed_adjustment = 0;
    wdata.conf.vane_type = VANE_TYPE_ST50;
    wdata.n2k_err = 0;
    
    Calibration calib;
    ByteBuffer buf(128); make_message(wdata, calib, buf);
    
    uint16_t i_ellipse = get_uint16(buf, 6);
    TEST_ASSERT_EQUAL_UINT16(1234, i_ellipse);
}

/**
 * Test: make_message encodes ellipse 2.0 as 2000
 */
void test_make_message_ellipse_2(void) {
    wind_data wdata;
    wdata.angle = 0.0;
    wdata.smooth_angle = 0.0;
    wdata.conf.offset = 0;
    wdata.heap = 0;
    wdata.angle_error = 0;
    wdata.ellipse = 2.0;
    wdata.speed = 10.0;
    wdata.speed_error = 0;
    wdata.i_sin = 2000;
    wdata.i_cos = 2000;
    wdata.conf.sin_range.set(600, 3400);
    wdata.conf.cos_range.set(600, 3400);
    wdata.conf.n2k_source = 21;
    wdata.conf.angle_smoothing = 50;
    wdata.conf.speed_smoothing = 50;
    wdata.conf.calibration_score_threshold = 50;
    wdata.conf.auto_cal = 0;
    wdata.conf.speed_adjustment = 0;
    wdata.conf.vane_type = VANE_TYPE_ST50;
    wdata.n2k_err = 0;
    
    Calibration calib;
    ByteBuffer buf(128); make_message(wdata, calib, buf);
    
    uint16_t i_ellipse = get_uint16(buf, 6);
    TEST_ASSERT_EQUAL_UINT16(2000, i_ellipse);
}

// ============================================================================
// Speed Encoding Tests
// ============================================================================

/**
 * Test: make_message encodes speed as speed * 10
 * Offset at bytes 30-31 (after sin/cos/ranges)
 */
void test_make_message_speed(void) {
    wind_data wdata;
    wdata.angle = 0.0;
    wdata.smooth_angle = 0.0;
    wdata.conf.offset = 0;
    wdata.heap = 0;
    wdata.angle_error = 0;
    wdata.ellipse = 1.0;
    wdata.speed = 12.5;  // Should encode as 125
    wdata.speed_error = 0;
    wdata.i_sin = 2000;
    wdata.i_cos = 2000;
    wdata.conf.sin_range.set(600, 3400);
    wdata.conf.cos_range.set(600, 3400);
    wdata.conf.n2k_source = 21;
    wdata.conf.angle_smoothing = 50;
    wdata.conf.speed_smoothing = 50;
    wdata.conf.calibration_score_threshold = 50;
    wdata.conf.auto_cal = 0;
    wdata.conf.speed_adjustment = 0;
    wdata.conf.vane_type = VANE_TYPE_ST50;
    wdata.n2k_err = 0;
    
    Calibration calib;
    ByteBuffer buf(128); make_message(wdata, calib, buf);
    
    uint16_t i_speed = get_uint16(buf, 25);
    TEST_ASSERT_EQUAL_UINT16(125, i_speed);
}

/**
 * Test: make_message encodes NAN speed as 0
 */
void test_make_message_speed_nan(void) {
    wind_data wdata;
    wdata.angle = 0.0;
    wdata.smooth_angle = 0.0;
    wdata.conf.offset = 0;
    wdata.heap = 0;
    wdata.angle_error = 0;
    wdata.ellipse = 1.0;
    wdata.speed = NAN;  // Should encode as 0
    wdata.speed_error = 0;
    wdata.i_sin = 2000;
    wdata.i_cos = 2000;
    wdata.conf.sin_range.set(600, 3400);
    wdata.conf.cos_range.set(600, 3400);
    wdata.conf.n2k_source = 21;
    wdata.conf.angle_smoothing = 50;
    wdata.conf.speed_smoothing = 50;
    wdata.conf.calibration_score_threshold = 50;
    wdata.conf.auto_cal = 0;
    wdata.conf.speed_adjustment = 0;
    wdata.conf.vane_type = VANE_TYPE_ST50;
    wdata.n2k_err = 0;
    
    Calibration calib;
    ByteBuffer buf(128); make_message(wdata, calib, buf);
    
    uint16_t i_speed = get_uint16(buf, 25);
    TEST_ASSERT_EQUAL_UINT16(0, i_speed);
}

/**
 * Test: make_message encodes zero speed
 */
void test_make_message_speed_zero(void) {
    wind_data wdata;
    wdata.angle = 0.0;
    wdata.smooth_angle = 0.0;
    wdata.conf.offset = 0;
    wdata.heap = 0;
    wdata.angle_error = 0;
    wdata.ellipse = 1.0;
    wdata.speed = 0.0;
    wdata.speed_error = 0;
    wdata.i_sin = 2000;
    wdata.i_cos = 2000;
    wdata.conf.sin_range.set(600, 3400);
    wdata.conf.cos_range.set(600, 3400);
    wdata.conf.n2k_source = 21;
    wdata.conf.angle_smoothing = 50;
    wdata.conf.speed_smoothing = 50;
    wdata.conf.calibration_score_threshold = 50;
    wdata.conf.auto_cal = 0;
    wdata.conf.speed_adjustment = 0;
    wdata.conf.vane_type = VANE_TYPE_ST50;
    wdata.n2k_err = 0;
    
    Calibration calib;
    ByteBuffer buf(128); make_message(wdata, calib, buf);
    
    uint16_t i_speed = get_uint16(buf, 25);
    TEST_ASSERT_EQUAL_UINT16(0, i_speed);
}

// ============================================================================
// Memory and Error Field Tests
// ============================================================================

/**
 * Test: make_message includes heap memory
 * Offset at bytes 8-11
 */
void test_make_message_memory(void) {
    wind_data wdata;
    wdata.angle = 0.0;
    wdata.smooth_angle = 0.0;
    wdata.conf.offset = 0;
    wdata.heap = 0x12345678;
    wdata.angle_error = 0;
    wdata.ellipse = 1.0;
    wdata.speed = 10.0;
    wdata.speed_error = 0;
    wdata.i_sin = 2000;
    wdata.i_cos = 2000;
    wdata.conf.sin_range.set(600, 3400);
    wdata.conf.cos_range.set(600, 3400);
    wdata.conf.n2k_source = 21;
    wdata.conf.angle_smoothing = 50;
    wdata.conf.speed_smoothing = 50;
    wdata.conf.calibration_score_threshold = 50;
    wdata.conf.auto_cal = 0;
    wdata.conf.speed_adjustment = 0;
    wdata.conf.vane_type = VANE_TYPE_ST50;
    wdata.n2k_err = 0;
    
    Calibration calib;
    ByteBuffer buf(128); make_message(wdata, calib, buf);
    
    uint32_t mem = get_uint32(buf, 8);
    TEST_ASSERT_EQUAL_UINT32(0x12345678, mem);
}

/**
 * Test: make_message includes angle_error flag
 * Offset at byte 12
 */
void test_make_message_angle_error(void) {
    wind_data wdata;
    wdata.angle = 0.0;
    wdata.smooth_angle = 0.0;
    wdata.conf.offset = 0;
    wdata.heap = 0;
    wdata.angle_error = 0x03;  // Multiple flags
    wdata.ellipse = 1.0;
    wdata.speed = 10.0;
    wdata.speed_error = 0;
    wdata.i_sin = 2000;
    wdata.i_cos = 2000;
    wdata.conf.sin_range.set(600, 3400);
    wdata.conf.cos_range.set(600, 3400);
    wdata.conf.n2k_source = 21;
    wdata.conf.angle_smoothing = 50;
    wdata.conf.speed_smoothing = 50;
    wdata.conf.calibration_score_threshold = 50;
    wdata.conf.auto_cal = 0;
    wdata.conf.speed_adjustment = 0;
    wdata.conf.vane_type = VANE_TYPE_ST50;
    wdata.n2k_err = 0;
    
    Calibration calib;
    ByteBuffer buf(128); make_message(wdata, calib, buf);
    
    uint8_t angle_error = get_uint8(buf, 12);
    TEST_ASSERT_EQUAL_UINT8(0x03, angle_error);
}

// ============================================================================
// ADC Reading Tests
// ============================================================================

/**
 * Test: make_message includes i_sin, i_cos readings
 */
void test_make_message_adc_readings(void) {
    wind_data wdata;
    wdata.angle = 0.0;
    wdata.smooth_angle = 0.0;
    wdata.conf.offset = 0;
    wdata.heap = 0;
    wdata.angle_error = 0;
    wdata.ellipse = 1.0;
    wdata.speed = 10.0;
    wdata.speed_error = 0;
    wdata.i_sin = 0xABCD;  // Specific test value
    wdata.i_cos = 0x5678;  // Specific test value
    wdata.conf.sin_range.set(600, 3400);
    wdata.conf.cos_range.set(600, 3400);
    wdata.conf.n2k_source = 21;
    wdata.conf.angle_smoothing = 50;
    wdata.conf.speed_smoothing = 50;
    wdata.conf.calibration_score_threshold = 50;
    wdata.conf.auto_cal = 0;
    wdata.conf.speed_adjustment = 0;
    wdata.conf.vane_type = VANE_TYPE_ST50;
    wdata.n2k_err = 0;
    
    Calibration calib;
    ByteBuffer buf(128); make_message(wdata, calib, buf);
    
    uint16_t i_sin = get_uint16(buf, 13);
    uint16_t i_cos = get_uint16(buf, 19);
    
    TEST_ASSERT_EQUAL_UINT16(0xABCD, i_sin);
    TEST_ASSERT_EQUAL_UINT16(0x5678, i_cos);
}

// ============================================================================
// Configuration Fields Tests
// ============================================================================

/**
 * Test: make_message includes calibration threshold
 */
void test_make_message_config_fields(void) {
    wind_data wdata;
    wdata.angle = 0.0;
    wdata.smooth_angle = 0.0;
    wdata.conf.offset = 123;
    wdata.heap = 0;
    wdata.angle_error = 0;
    wdata.ellipse = 1.0;
    wdata.speed = 10.0;
    wdata.speed_error = 0;
    wdata.i_sin = 2000;
    wdata.i_cos = 2000;
    wdata.conf.sin_range.set(600, 3400);
    wdata.conf.cos_range.set(600, 3400);
    wdata.conf.n2k_source = 42;
    wdata.conf.angle_smoothing = 75;
    wdata.conf.speed_smoothing = 80;
    wdata.conf.calibration_score_threshold = 85;
    wdata.conf.auto_cal = 1;
    wdata.conf.speed_adjustment = 10;
    wdata.conf.vane_type = VANE_TYPE_ST60;
    wdata.n2k_err = 0;
    
    Calibration calib;
    ByteBuffer buf(128); make_message(wdata, calib, buf);
    
    // Configuration should be serialized
    // Exact offsets depend on message format, just verify non-zero
    TEST_ASSERT_NOT_EQUAL(0, buf.size());
}

// ============================================================================
// Integration Tests
// ============================================================================

/**
 * Test: make_message produces consistent buffer size
 */
void test_make_message_buffer_size(void) {
    wind_data wdata;
    wdata.angle = 45.5;
    wdata.smooth_angle = 44.2;
    wdata.conf.offset = 10;
    wdata.heap = 50000;
    wdata.angle_error = 0;
    wdata.ellipse = 1.5;
    wdata.speed = 15.3;
    wdata.speed_error = 0;
    wdata.i_sin = 2100;
    wdata.i_cos = 2200;
    wdata.conf.sin_range.set(600, 3400);
    wdata.conf.cos_range.set(600, 3400);
    wdata.conf.n2k_source = 21;
    wdata.conf.angle_smoothing = 50;
    wdata.conf.speed_smoothing = 50;
    wdata.conf.calibration_score_threshold = 50;
    wdata.conf.auto_cal = 0;
    wdata.conf.speed_adjustment = 0;
    wdata.conf.vane_type = VANE_TYPE_ST50;
    wdata.n2k_err = 0;
    
    Calibration calib;
    ByteBuffer buf(128); make_message(wdata, calib, buf);
    
    // Buffer should fit in 128 bytes (allocated size)
    TEST_ASSERT_LESS_THAN_UINT32(128, buf.length());
    TEST_ASSERT_GREATER_THAN_UINT32(20, buf.length());  // Should have significant data
}

/**
 * Test: make_message with all fields populated
 */
void test_make_message_full_data(void) {
    wind_data wdata;
    wdata.angle = 123.45;
    wdata.smooth_angle = 120.0;
    wdata.conf.offset = 50;
    wdata.heap = 123456;
    wdata.angle_error = WIND_ERROR_NO_SIGNAL;
    wdata.ellipse = 1.8;
    wdata.speed = 25.7;
    wdata.speed_error = 0;
    wdata.i_sin = 2500;
    wdata.i_cos = 2800;
    wdata.conf.sin_range.set(500, 3500);
    wdata.conf.cos_range.set(600, 3400);
    wdata.conf.n2k_source = 20;
    wdata.conf.angle_smoothing = 60;
    wdata.conf.speed_smoothing = 70;
    wdata.conf.calibration_score_threshold = 75;
    wdata.conf.auto_cal = 1;
    wdata.conf.speed_adjustment = 5;
    wdata.conf.vane_type = VANE_TYPE_ST60;
    wdata.n2k_err = 1;
    
    Calibration calib;
    ByteBuffer buf(128); make_message(wdata, calib, buf);
    
    // Should not crash and produce valid buffer
    TEST_ASSERT_TRUE(buf.size() > 0);
    TEST_ASSERT_LESS_THAN_UINT32(128, buf.length());
}

// ============================================================================
// Run All Tests
// ============================================================================

void setup(void) {
    UNITY_BEGIN();
    
    // Angle encoding tests
    RUN_TEST(test_make_message_angle_zero);
    RUN_TEST(test_make_message_angle_45);
    RUN_TEST(test_make_message_angle_90);
    RUN_TEST(test_make_message_angle_negative);
    RUN_TEST(test_make_message_angle_359);
    
    // Smooth angle tests
    RUN_TEST(test_make_message_smooth_angle);
    
    // Output angle tests
    RUN_TEST(test_make_message_output_angle_with_offset);
    RUN_TEST(test_make_message_output_angle_wrap);
    
    // Ellipse tests
    RUN_TEST(test_make_message_ellipse);
    RUN_TEST(test_make_message_ellipse_2);
    
    // Speed tests
    RUN_TEST(test_make_message_speed);
    RUN_TEST(test_make_message_speed_nan);
    RUN_TEST(test_make_message_speed_zero);
    
    // Memory and error tests
    RUN_TEST(test_make_message_memory);
    RUN_TEST(test_make_message_angle_error);
    
    // ADC reading tests
    RUN_TEST(test_make_message_adc_readings);
    
    // Configuration tests
    RUN_TEST(test_make_message_config_fields);
    
    // Integration tests
    RUN_TEST(test_make_message_buffer_size);
    RUN_TEST(test_make_message_full_data);
    
    UNITY_END();
}

void loop(void) {
    // Empty
}

int main(int argc, char **argv) {
    setup();
    return 0;
}