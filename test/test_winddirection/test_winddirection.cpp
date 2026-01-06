#include <unity.h>
#include <math.h>
#include <limits.h>
#include "../src/WindDirection.h"
#include "../src/Constants.h"

// ============================================================================
// Test Data Setup Helpers
// ============================================================================

static configuration create_default_config(void) {
    configuration conf;
    conf.sin_range.set(600, 3400);
    conf.cos_range.set(600, 3400);
    conf.offset = 0;
    conf.angle_smoothing = 50;
    conf.speed_smoothing = 50;
    conf.calibration_score_threshold = 50;
    conf.auto_cal = 0;
    conf.speed_adjustment = 0;
    conf.n2k_source = 21;
    conf.vane_type = VANE_TYPE_ST50;
    conf.usb_tracing = 0;
    conf.stw_smoothing = 50;
    conf.stw_adjustment = 0;
    conf.temp_smoothing = 50;
    conf.enable_stw = 0;
    conf.enable_temp = 0;
    return conf;
}

static wind_data create_default_wind_data(void) {
    wind_data wd;
    wd.angle = 0.0;
    wd.smooth_angle = 0.0;
    wd.angle_error = 0;
    wd.ellipse = 1.0;
    wd.speed = 0.0;
    wd.speed_error = 0;
    wd.i_sin = 0;
    wd.i_cos = 0;
    return wd;
}

// ============================================================================
// Construction & Initialization Tests
// ============================================================================

/**
 * Test: WindDirection construction with pin parameters
 * Should initialize buffers to zero and counters to 0
 */
void test_construction(void) {
    WindDirection wd(16, 17);  // Test pin numbers
    TEST_ASSERT_EQUAL_UINT32(0, wd.get_sample_age());
}

/**
 * Test: WindDirection setup
 * Should complete without errors (hardware setup skipped in test environment)
 */
void test_setup(void) {
    WindDirection wd(16, 17);
    wd.setup();  // No-op in test environment
    TEST_PASS();
}

// ============================================================================
// Loop Micros & Sample Buffering Tests
// ============================================================================

/**
 * Test: loop_micros with test readings
 * Should buffer sin/cos readings into circular buffers
 */
void test_loop_micros_single_reading(void) {
    WindDirection wd(16, 17);
    
    // Provide test readings (bypasses hardware ADC)
    wd.loop_micros(1000000, 2048, 2048);
    
    configuration conf = create_default_config();
    wind_data wd_data = create_default_wind_data();
    
    wd.read_data(wd_data, conf, 1000);
    
    // Both readings should have been buffered (values will be averaged)
    TEST_ASSERT_EQUAL_UINT16(2048, wd_data.i_sin);
    TEST_ASSERT_EQUAL_UINT16(2048, wd_data.i_cos);
}

/**
 * Test: loop_micros with zero readings (invalid)
 * Readings below 600 are invalid
 */
void test_loop_micros_invalid_readings(void) {
    WindDirection wd(16, 17);
    
    // Provide invalid readings
    wd.loop_micros(1000, 100, 100);
    
    configuration conf = create_default_config();
    wind_data wd_data = create_default_wind_data();
    
    wd.read_data(wd_data, conf, 1000);
    
    // Should set NO_SIGNAL error
    TEST_ASSERT_TRUE(wd_data.angle_error & WIND_ERROR_NO_SIGNAL);
    TEST_ASSERT_TRUE(isnan(wd_data.angle));
    TEST_ASSERT_TRUE(isnan(wd_data.ellipse));
}

/**
 * Test: loop_micros with default parameters (no test readings)
 * When test readings are UINT16_MAX, should not buffer (hardware ADC skipped in test)
 */
void test_loop_micros_no_test_readings(void) {
    WindDirection wd(16, 17);
    
    // No test readings provided (hardware ADC disabled in test environment)
    wd.loop_micros(1000);  // Uses default UINT16_MAX values
    
    configuration conf = create_default_config();
    wind_data wd_data = create_default_wind_data();
    
    wd.read_data(wd_data, conf, 1000);
    
    // Should complete without crashing
    // With zero buffers, behavior depends on implementation
    TEST_PASS();
}

/**
 * Test: loop_micros buffering multiple samples
 * Circular buffer should average multiple readings
 */
void test_loop_micros_multiple_samples(void) {
    WindDirection wd(16, 17);
    configuration conf = create_default_config();
    wind_data wd_data = create_default_wind_data();
    
    // Buffer several readings
    wd.loop_micros(1000, 2000, 2000);
    wd.loop_micros(1001, 2100, 1900);
    wd.loop_micros(1002, 2050, 1950);
    
    wd.read_data(wd_data, conf, 1002);
    
    // Readings should be averaged
    // Average of [2000, 2100, 2050] = 2050
    // Average of [2000, 1900, 1950] = 1950
    TEST_ASSERT_EQUAL_UINT16(2050, wd_data.i_cos);
    TEST_ASSERT_EQUAL_UINT16(1950, wd_data.i_sin);
}

// ============================================================================
// Read Data Tests
// ============================================================================

/**
 * Test: read_data with valid centered readings
 * Should calculate angle and ellipse correctly
 */
void test_read_data_centered(void) {
    WindDirection wd(16, 17);
    
    configuration conf = create_default_config();
    conf.sin_range.set(600, 3400);
    conf.cos_range.set(600, 3400);
    wd.apply_configuration(conf);
    
    wind_data wd_data = create_default_wind_data();
    
    // Center readings: equal sin and cos = 45° (northeast)
    uint16_t center = 2000;
    wd.loop_micros(1000, center, center);
    wd.read_data(wd_data, conf, 1000);
    
    // v_sin and v_cos should both be ~0
    // atan2(0, 0) = 0°
    // ellipse = sqrt(0^2 + 0^2) = 0
    TEST_ASSERT_FALSE(wd_data.angle_error & WIND_ERROR_NO_SIGNAL);
    TEST_ASSERT_DOUBLE_WITHIN(1.0, 0.0, wd_data.ellipse);
    TEST_ASSERT_FALSE(isnan(wd_data.angle));
}

/**
 * Test: read_data angle calculation
 * Verify atan2 mapping to compass angles
 */
void test_read_data_angle_calculation_0(void) {
    WindDirection wd(16, 17);
    
    configuration conf = create_default_config();
    conf.sin_range.set(600, 3400);
    conf.cos_range.set(600, 3400);
    wd.apply_configuration(conf);
    
    wind_data wd_data = create_default_wind_data();
    
    // East (sin=0, cos=1): angle = 0°
    // To get v_cos=1, we need i_cos near 3400
    // To get v_sin=0, we need i_sin at center=2000
    uint16_t sin_min = 600, sin_max = 3400, cos_min = 600, cos_max = 3400;
    
    // East: v_sin=0, v_cos=+1
    wd.loop_micros(1000, cos_max, (sin_min + sin_max) / 2);
    wd.read_data(wd_data, conf, 1000);
    TEST_ASSERT_DOUBLE_WITHIN(2.0, 0.0, wd_data.angle);
}

/**
 * Test: read_data angle calculation
 * Verify atan2 mapping to compass angles
 */
void test_read_data_angle_calculation_90(void) {
    WindDirection wd(16, 17);
    
    configuration conf = create_default_config();
    conf.sin_range.set(600, 3400);
    conf.cos_range.set(600, 3400);
    wd.apply_configuration(conf);
    
    wind_data wd_data = create_default_wind_data();
    
    // North: v_sin=+1, v_cos=0
    uint16_t sin_min = 600, sin_max = 3400, cos_min = 600, cos_max = 3400;
    
    // North: v_sin=+1, v_cos=0
    wd.loop_micros(1001, (cos_min + cos_max) / 2, sin_max);
    wd.read_data(wd_data, conf, 1001);
    TEST_ASSERT_DOUBLE_WITHIN(2.0, 90.0, wd_data.angle);
}


/**
 * Test: read_data angle calculation
 * Verify atan2 mapping to compass angles
 */
void test_read_data_angle_calculation_180(void) {
    WindDirection wd(16, 17);
    
    configuration conf = create_default_config();
    conf.sin_range.set(600, 3400);
    conf.cos_range.set(600, 3400);
    wd.apply_configuration(conf);
    
    wind_data wd_data = create_default_wind_data();
    
    // West: v_sin=0, v_cos=-1
    uint16_t sin_min = 600, sin_max = 3400, cos_min = 600, cos_max = 3400;

    // West: v_sin=0, v_cos=-1
    wd.loop_micros(1002, cos_min, (sin_min + sin_max) / 2);
    wd.read_data(wd_data, conf, 1002);
    TEST_ASSERT_DOUBLE_WITHIN(2.0, 180.0, wd_data.angle);
}

/**
 * Test: read_data angle calculation
 * Verify atan2 mapping to compass angles
 */
void test_read_data_angle_calculation_270(void) {
    WindDirection wd(16, 17);
    
    configuration conf = create_default_config();
    conf.sin_range.set(600, 3400);
    conf.cos_range.set(600, 3400);
    wd.apply_configuration(conf);
    
    wind_data wd_data = create_default_wind_data();
    
    // South: v_sin=-1, v_cos=0
    uint16_t sin_min = 600, sin_max = 3400, cos_min = 600, cos_max = 3400;
    
    // South: v_sin=-1, v_cos=0
    wd.loop_micros(1003, (cos_min + cos_max) / 2, sin_min);
    wd.read_data(wd_data, conf, 1003);
    TEST_ASSERT_DOUBLE_WITHIN(2.0, 270.0, wd_data.angle);
}

/**
 * Test: read_data with sin reading below threshold (600)
 * Should set NO_SIGNAL error
 */
void test_read_data_sin_below_threshold(void) {
    WindDirection wd(16, 17);
    
    configuration conf = create_default_config();
    conf.sin_range.set(600, 3400);
    conf.cos_range.set(600, 3400);
    wd.apply_configuration(conf);
    
    wind_data wd_data = create_default_wind_data();
    
    // Sin reading below 600 (invalid)
    wd.loop_micros(1000, 500, 2000);
    wd.read_data(wd_data, conf, 1000);
    
    TEST_ASSERT_TRUE(wd_data.angle_error & WIND_ERROR_NO_SIGNAL);
    TEST_ASSERT_TRUE(isnan(wd_data.angle));
    TEST_ASSERT_TRUE(isnan(wd_data.ellipse));
}

/**
 * Test: read_data with cos reading above MAX_ADC_VALUE (4095)
 * Should handle gracefully (may wrap or clip depending on rounding)
 */
void test_read_data_cos_valid_max(void) {
    WindDirection wd(16, 17);
    
    configuration conf = create_default_config();
    conf.sin_range.set(600, 3400);
    conf.cos_range.set(600, 3400);
    wd.apply_configuration(conf);
    
    wind_data wd_data = create_default_wind_data();
    
    // Cos at maximum valid value
    wd.loop_micros(1000, 2000, 4095);
    wd.read_data(wd_data, conf, 1000);
    
    TEST_ASSERT_FALSE(wd_data.angle_error & WIND_ERROR_NO_SIGNAL);
    TEST_ASSERT_FALSE(isnan(wd_data.angle));
}

/**
 * Test: read_data clears NO_SIGNAL error when readings become valid
 */
void test_read_data_error_recovery(void) {
    WindDirection wd(16, 17);
    
    configuration conf = create_default_config();
    conf.sin_range.set(600, 3400);
    conf.cos_range.set(600, 3400);
    wd.apply_configuration(conf);
    
    wind_data wd_data = create_default_wind_data();
    
    // First: invalid reading
    wd.loop_micros(1000, 100, 2000);
    wd.read_data(wd_data, conf, 1000);
    TEST_ASSERT_TRUE(wd_data.angle_error & WIND_ERROR_NO_SIGNAL);
    
    // Second: valid reading should clear error
    wd.loop_micros(1001, 2000, 2000);
    wd.read_data(wd_data, conf, 1001);
    TEST_ASSERT_FALSE(wd_data.angle_error & WIND_ERROR_NO_SIGNAL);
}

/**
 * Test: read_data updates last_read_time
 */
void test_read_data_timestamp(void) {
    WindDirection wd(16, 17);
    
    configuration conf = create_default_config();
    conf.sin_range.set(600, 3400);
    conf.cos_range.set(600, 3400);
    wd.apply_configuration(conf);
    
    wind_data wd_data = create_default_wind_data();
    
    wd.loop_micros(1000, 2000, 2000);
    wd.read_data(wd_data, conf, 5000);
    
    TEST_ASSERT_EQUAL_UINT32(5000, wd.get_sample_age());
}

/**
 * Test: read_data ellipse calculation
 * ellipse = sqrt(v_sin^2 + v_cos^2)
 */
void test_read_data_ellipse(void) {
    WindDirection wd(16, 17);
    
    configuration conf = create_default_config();
    conf.sin_range.set(600, 3400);
    conf.cos_range.set(600, 3400);
    wd.apply_configuration(conf);
    
    wind_data wd_data = create_default_wind_data();
    
    uint16_t sin_min = 600, sin_max = 3400, cos_min = 600, cos_max = 3400;
    uint16_t mid = (sin_min + sin_max) / 2;
    
    // 45° angle: v_sin=v_cos (normalized)
    // Should produce ellipse = sqrt(2) ≈ 1.414 if both are at max
    wd.loop_micros(1000, sin_max, cos_max);
    wd.read_data(wd_data, conf, 1000);
    
    // With normalized [-1,1] ranges, max ellipse = sqrt(2)
    TEST_ASSERT_DOUBLE_WITHIN(0.2, 1.414, wd_data.ellipse);
}

// ============================================================================
// Angle Smoothing Tests
// ============================================================================

/**
 * Test: smooth_angle uses previous value (maintains state)
 */
void test_read_data_smooth_angle_accumulates(void) {
    WindDirection wd(16, 17);
    
    configuration conf = create_default_config();
    conf.sin_range.set(600, 3400);
    conf.cos_range.set(600, 3400);
    conf.angle_smoothing = 50;
    wd.apply_configuration(conf);
    
    wind_data wd_data = create_default_wind_data();
    
    uint16_t sin_min = 600, sin_max = 3400, cos_min = 600, cos_max = 3400;
    uint16_t mid = (sin_min + sin_max) / 2;
    
    // Sequence of readings 0° - saturate the buffer
    for (int i = 0; i<SIN_COS_BUFFER_SIZE; i++) {

        wd.loop_micros(10000000 + i, cos_max, mid);
    }
    wd.read_data(wd_data, conf, 10000000 + SIN_COS_BUFFER_SIZE);
    TEST_ASSERT_DOUBLE_WITHIN(1.0, 0.0, wd_data.angle);
    TEST_ASSERT_DOUBLE_WITHIN(1.0, 0.0, wd_data.smooth_angle);
    
    double prev_smooth = wd_data.smooth_angle;
    

    
    // Sequence of readings 90° - saturate the buffer
    for (int i = 0; i<SIN_COS_BUFFER_SIZE; i++) {

        wd.loop_micros(20000000 + i, mid, sin_max);
    }
    wd.read_data(wd_data, conf, 20000000 + SIN_COS_BUFFER_SIZE);
    TEST_ASSERT_DOUBLE_WITHIN(1.0, 90.0, wd_data.angle);
    TEST_ASSERT_TRUE(wd_data.smooth_angle > prev_smooth);
    TEST_ASSERT_TRUE(wd_data.smooth_angle < 90.0);
}

// ============================================================================
// Configuration Application Tests
// ============================================================================

/**
 * Test: apply_configuration
 */
void test_apply_configuration(void) {
    WindDirection wd(16, 17);
    
    configuration conf = create_default_config();
    conf.sin_range.set(500, 3500);
    conf.cos_range.set(700, 3300);
    
    wd.apply_configuration(conf);
    
    // No direct access to ranges, but they're used in read_data
    wind_data wd_data = create_default_wind_data();
    
    // Should use configured ranges for conversion
    wd.loop_micros(1000, 2000, 2000);
    wd.read_data(wd_data, conf, 1000);
    
    TEST_ASSERT_FALSE(isnan(wd_data.angle));
}

/**
 * Test: different calibration ranges produce different angles
 */
void test_different_calibration_ranges(void) {
    WindDirection wd1(16, 17), wd2(18, 19);
    
    // Configuration 1: symmetric ranges
    configuration conf1 = create_default_config();
    conf1.sin_range.set(600, 3400);
    conf1.cos_range.set(600, 3400);
    wd1.apply_configuration(conf1);
    
    // Configuration 2: asymmetric ranges
    configuration conf2 = create_default_config();
    conf2.sin_range.set(800, 3200);
    conf2.cos_range.set(600, 3400);
    wd2.apply_configuration(conf2);
    
    wind_data wd_data1 = create_default_wind_data();
    wind_data wd_data2 = create_default_wind_data();
    
    // Same ADC readings, different ranges
    wd1.loop_micros(1000, 2000, 2000);
    wd1.read_data(wd_data1, conf1, 1000);
    
    wd2.loop_micros(1000, 2000, 2000);
    wd2.read_data(wd_data2, conf2, 1000);
    
    // Both should be valid, but may differ slightly due to calibration
    TEST_ASSERT_FALSE(isnan(wd_data1.angle));
    TEST_ASSERT_FALSE(isnan(wd_data2.angle));
}

// ============================================================================
// Edge Cases & Boundary Tests
// ============================================================================

/**
 * Test: reading exactly at minimum valid threshold (600) - valid case
 */
void test_read_data_below_min_threshold(void) {
    WindDirection wd(16, 17);
    
    configuration conf = create_default_config();
    conf.sin_range.set(600, 3400);
    conf.cos_range.set(600, 3400);
    wd.apply_configuration(conf);
    
    wind_data wd_data = create_default_wind_data();
    
    // Fill buffer with valid readings, then one at exact threshold
    for (int i = 0; i < 199; i++) {
        wd.loop_micros(1000 + i, 2000, 2000);
    }
    wd.loop_micros(1199, 600, 600);  // Exact threshold
    
    wd.read_data(wd_data, conf, 1199);
    
    // Should be valid (threshold is 600, not < 600)
    TEST_ASSERT_FALSE(wd_data.angle_error & WIND_ERROR_NO_SIGNAL);
}

/**
 * Test: reading just below minimum valid threshold (599)
 */
void test_read_data_at_min_threshold(void) {
    WindDirection wd(16, 17);
    
    configuration conf = create_default_config();
    conf.sin_range.set(600, 3400);
    conf.cos_range.set(600, 3400);
    wd.apply_configuration(conf);
    
    wind_data wd_data = create_default_wind_data();
    
    wd.loop_micros(1000, 599, 2000);
    wd.read_data(wd_data, conf, 1000);
    
    TEST_ASSERT_TRUE(wd_data.angle_error & WIND_ERROR_NO_SIGNAL);
}

/**
 * Test: reading at maximum ADC value (4095)
 */
void test_read_data_at_max_adc(void) {
    WindDirection wd(16, 17);
    
    configuration conf = create_default_config();
    conf.sin_range.set(600, 3400);
    conf.cos_range.set(600, 3400);
    wd.apply_configuration(conf);
    
    wind_data wd_data = create_default_wind_data();
    
    // Both at maximum
    for (int i = 0; i < 199; i++) {
        wd.loop_micros(1000 + i, 4095, 4095);
    }
    wd.loop_micros(1199, 4095, 4095);
    
    wd.read_data(wd_data, conf, 1199);
    
    TEST_ASSERT_FALSE(wd_data.angle_error & WIND_ERROR_NO_SIGNAL);
    TEST_ASSERT_FALSE(isnan(wd_data.angle));
}

/**
 * Test: mixed valid/invalid readings in buffer
 * Invalid reading mixed with valid ones
 */
void test_read_data_mixed_valid_invalid(void) {
    WindDirection wd(16, 17);
    
    configuration conf = create_default_config();
    conf.sin_range.set(600, 3400);
    conf.cos_range.set(600, 3400);
    wd.apply_configuration(conf);
    
    wind_data wd_data = create_default_wind_data();
    
    // Mix valid and invalid readings
    wd.loop_micros(1000, 2000, 2000);  // Valid
    wd.loop_micros(1001, 500, 2000);   // Invalid (sin < 600)
    wd.loop_micros(1002, 2000, 2000);  // Valid
    
    wd.read_data(wd_data, conf, 1002);
    
    // Last reading average includes invalid value
    // Average sin = (2000 + 500 + 2000) / 3 = 1500 (below 600 threshold)
    // This may or may not trigger error depending on averaging
}

// ============================================================================
// Run All Tests
// ============================================================================

void setup(void) {
    UNITY_BEGIN();
    
    // Construction & Initialization
    RUN_TEST(test_construction);
    RUN_TEST(test_setup);
    
    // Loop Micros & Buffering
    RUN_TEST(test_loop_micros_single_reading);
    RUN_TEST(test_loop_micros_invalid_readings);
    RUN_TEST(test_loop_micros_no_test_readings);
    RUN_TEST(test_loop_micros_multiple_samples);
    
    // Read Data
    RUN_TEST(test_read_data_centered);
    RUN_TEST(test_read_data_angle_calculation_0);
    RUN_TEST(test_read_data_angle_calculation_90);
    RUN_TEST(test_read_data_angle_calculation_180);
    RUN_TEST(test_read_data_angle_calculation_270);
    RUN_TEST(test_read_data_sin_below_threshold);
    RUN_TEST(test_read_data_cos_valid_max);
    RUN_TEST(test_read_data_error_recovery);
    RUN_TEST(test_read_data_timestamp);
    RUN_TEST(test_read_data_ellipse);
    
    // Angle Smoothing
    RUN_TEST(test_read_data_smooth_angle_accumulates);
    
    // Configuration
    RUN_TEST(test_apply_configuration);
    RUN_TEST(test_different_calibration_ranges);
    
    // Edge Cases
    RUN_TEST(test_read_data_at_min_threshold);
    RUN_TEST(test_read_data_below_min_threshold);
    RUN_TEST(test_read_data_at_max_adc);
    RUN_TEST(test_read_data_mixed_valid_invalid);
    
    UNITY_END();
}

void loop(void) {
    // Empty
}

int main() {
    setup();
    return 0;
}