#include <unity.h>
#include <math.h>
#include <limits.h>
#include "../src/BLESerializer.h"
#include "../src/DataAndConf.h"
#include "../src/Calibration.h"
#include "../src/Constants.h"
#include "../src/WindCorrection.h"
#include "../src/CorrectionProtocol.h"

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * Extract uint16_t from buffer at offset
 */
static uint16_t get_uint16(const ByteBuffer &buf, size_t offset) {
    uint8_t data[256];
    buf.get_data(data, 256);
    return (uint16_t)((data[offset + 1] << 8) | data[offset]);
}

/**
 * Extract uint32_t from buffer at offset
 */
static uint32_t get_uint32(const ByteBuffer &buf, size_t offset) {
    uint8_t data[256];
    buf.get_data(data, 256);
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
    uint8_t data[256];
    buf.get_data(data, 256);
    return data[offset];
}

// ============================================================================
// Test Data Setup Helpers
// ============================================================================

static configuration create_default_config(void) {
    configuration conf;
    conf.offset = 0;
    conf.speed_adjustment = 0;
    conf.angle_smoothing = 50;
    conf.speed_smoothing = 50;
    conf.calibration_score_threshold = 50;
    conf.auto_cal = 0;
    conf.usb_tracing = 0;
    conf.vane_type = VANE_TYPE_ST50;
    conf.n2k_source = 21;
    conf.sin_range.set(600, 3400);
    conf.cos_range.set(600, 3400);
    conf.set_ble_name("ABWind");
    return conf;
}

static all_data create_default_data(void) {
    all_data data;
    data.n2k_err = 0;
    data.heap = 0;
    data.wind.angle = 0.0;
    data.wind.smooth_angle = 0.0;
    data.wind.angle_error = 0;
    data.wind.ellipse = 1.0;
    data.wind.speed = 10.0;
    data.wind.speed_error = 0;
    data.wind.i_sin = 2000;
    data.wind.i_cos = 2000;
    return data;
}

// ============================================================================
// Angle Encoding Tests
// ============================================================================

/**
 * Test: make_message encodes angle as (angle * 10 + 3600) % 3600
 * 0° -> 0 (encoded as 0)
 */
void test_make_message_angle_zero(void) {
    configuration conf = create_default_config();
    all_data data = create_default_data();
    data.wind.angle = 0.0;
    data.wind.smooth_angle = 0.0;
    
    Calibration calib;
    ByteBuffer buf(256);
    make_message(conf, data, calib, buf);
    
    // i_angle = ((int16_t)(0.0 * 10 + 0.5) + 3600) % 3600 = (0 + 3600) % 3600 = 0
    uint16_t i_angle = get_uint16(buf, 0);
    TEST_ASSERT_EQUAL_UINT16(0, i_angle);
}

/**
 * Test: make_message encodes angle 45° correctly
 */
void test_make_message_angle_45(void) {
    configuration conf = create_default_config();
    all_data data = create_default_data();
    data.wind.angle = 45.0;
    data.wind.smooth_angle = 45.0;
    
    Calibration calib;
    ByteBuffer buf(256);
    make_message(conf, data, calib, buf);
    
    // i_angle = ((int16_t)(45.0 * 10 + 0.5) + 3600) % 3600 = (450 + 3600) % 3600 = 450
    uint16_t i_angle = get_uint16(buf, 0);
    TEST_ASSERT_EQUAL_UINT16(450, i_angle);
}

/**
 * Test: make_message encodes angle 90° correctly
 */
void test_make_message_angle_90(void) {
    configuration conf = create_default_config();
    all_data data = create_default_data();
    data.wind.angle = 90.0;
    data.wind.smooth_angle = 90.0;
    
    Calibration calib;
    ByteBuffer buf(256);
    make_message(conf, data, calib, buf);
    
    // i_angle = ((int16_t)(90.0 * 10 + 0.5) + 3600) % 3600 = (900 + 3600) % 3600 = 900
    uint16_t i_angle = get_uint16(buf, 0);
    TEST_ASSERT_EQUAL_UINT16(900, i_angle);
}

/**
 * Test: make_message wraps negative angles (e.g., -45° -> 3150)
 */
void test_make_message_angle_negative(void) {
    configuration conf = create_default_config();
    all_data data = create_default_data();
    data.wind.angle = -45.0;
    data.wind.smooth_angle = -45.0;
    
    Calibration calib;
    ByteBuffer buf(256);
    make_message(conf, data, calib, buf);
    
    // i_angle = ((int16_t)(-45.0 * 10 + 0.5) + 3600) % 3600 = (-450 + 3600) % 3600 = 3150
    uint16_t i_angle = get_uint16(buf, 0);
    // Allow ±1 for floating point rounding
    TEST_ASSERT_TRUE((i_angle >= 3149) && (i_angle <= 3151));
}

/**
 * Test: make_message encodes angle 359° correctly
 */
void test_make_message_angle_359(void) {
    configuration conf = create_default_config();
    all_data data = create_default_data();
    data.wind.angle = 359.0;
    data.wind.smooth_angle = 359.0;
    
    Calibration calib;
    ByteBuffer buf(256);
    make_message(conf, data, calib, buf);
    
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
    configuration conf = create_default_config();
    all_data data = create_default_data();
    data.wind.smooth_angle = 180.5;
    
    Calibration calib;
    ByteBuffer buf(256);
    make_message(conf, data, calib, buf);
    
    // i_smooth_angle = ((int16_t)(180.5 * 10 + 0.5) + 3600) % 3600 = (1805 + 3600) % 3600 = 1805
    uint16_t i_smooth_angle = get_uint16(buf, 2);
    TEST_ASSERT_EQUAL_UINT16(1805, i_smooth_angle);
}

// ============================================================================
// Output Angle (with offset) Tests
// ============================================================================

/**
 * Test: make_message calculates output_angle with offset
 */
void test_make_message_output_angle_with_offset(void) {
    configuration conf = create_default_config();
    conf.offset = 45;
    all_data data = create_default_data();
    data.wind.smooth_angle = 90.0;
    
    Calibration calib;
    ByteBuffer buf(256);
    make_message(conf, data, calib, buf);
    
    // i_output_angle = ((900 + 45 * 10) + 3600) % 3600 = (900 + 450 + 3600) % 3600 = 4950 % 3600 = 1350
    uint16_t i_output_angle = get_uint16(buf, 4);
    TEST_ASSERT_EQUAL_UINT16(1350, i_output_angle);
}

/**
 * Test: make_message output_angle wraps at 3600
 */
void test_make_message_output_angle_wrap(void) {
    configuration conf = create_default_config();
    conf.offset = 50;
    all_data data = create_default_data();
    data.wind.smooth_angle = 350.0;
    
    Calibration calib;
    ByteBuffer buf(256);
    make_message(conf, data, calib, buf);
    
    // i_output_angle = ((3500 + 50 * 10) + 3600) % 3600 = (3500 + 500 + 3600) % 3600 = 7600 % 3600 = 400
    uint16_t i_output_angle = get_uint16(buf, 4);
    TEST_ASSERT_EQUAL_UINT16(400, i_output_angle);
}

// ============================================================================
// Ellipse Encoding Tests
// ============================================================================

/**
 * Test: make_message encodes ellipse as ellipse * 1000
 */
void test_make_message_ellipse(void) {
    configuration conf = create_default_config();
    all_data data = create_default_data();
    data.wind.ellipse = 1.234;
    
    Calibration calib;
    ByteBuffer buf(256);
    make_message(conf, data, calib, buf);
    
    uint16_t i_ellipse = get_uint16(buf, 6);
    TEST_ASSERT_EQUAL_UINT16(1234, i_ellipse);
}

/**
 * Test: make_message encodes ellipse 2.0 as 2000
 */
void test_make_message_ellipse_2(void) {
    configuration conf = create_default_config();
    all_data data = create_default_data();
    data.wind.ellipse = 2.0;
    
    Calibration calib;
    ByteBuffer buf(256);
    make_message(conf, data, calib, buf);
    
    uint16_t i_ellipse = get_uint16(buf, 6);
    TEST_ASSERT_EQUAL_UINT16(2000, i_ellipse);
}

// ============================================================================
// Speed Encoding Tests
// ============================================================================

/**
 * Test: make_message encodes speed as speed * 10
 */
void test_make_message_speed(void) {
    configuration conf = create_default_config();
    all_data data = create_default_data();
    data.wind.speed = 12.5;
    
    Calibration calib;
    ByteBuffer buf(256);
    make_message(conf, data, calib, buf);
    
    uint16_t i_speed = get_uint16(buf, 25);
    TEST_ASSERT_EQUAL_UINT16(125, i_speed);
}

/**
 * Test: make_message encodes NAN speed as 0
 */
void test_make_message_speed_nan(void) {
    configuration conf = create_default_config();
    all_data data = create_default_data();
    data.wind.speed = NAN;
    
    Calibration calib;
    ByteBuffer buf(256);
    make_message(conf, data, calib, buf);
    
    uint16_t i_speed = get_uint16(buf, 25);
    TEST_ASSERT_EQUAL_UINT16(0, i_speed);
}

/**
 * Test: make_message encodes zero speed
 */
void test_make_message_speed_zero(void) {
    configuration conf = create_default_config();
    all_data data = create_default_data();
    data.wind.speed = 0.0;
    
    Calibration calib;
    ByteBuffer buf(256);
    make_message(conf, data, calib, buf);
    
    uint16_t i_speed = get_uint16(buf, 25);
    TEST_ASSERT_EQUAL_UINT16(0, i_speed);
}

// ============================================================================
// Memory and Error Field Tests
// ============================================================================

/**
 * Test: make_message includes heap memory
 */
void test_make_message_memory(void) {
    configuration conf = create_default_config();
    all_data data = create_default_data();
    data.heap = 0x12345678;
    
    Calibration calib;
    ByteBuffer buf(256);
    make_message(conf, data, calib, buf);
    
    uint32_t mem = get_uint32(buf, 8);
    TEST_ASSERT_EQUAL_UINT32(0x12345678, mem);
}

/**
 * Test: make_message includes angle_error flag
 */
void test_make_message_angle_error(void) {
    configuration conf = create_default_config();
    all_data data = create_default_data();
    data.wind.angle_error = 0x03;
    
    Calibration calib;
    ByteBuffer buf(256);
    make_message(conf, data, calib, buf);
    
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
    configuration conf = create_default_config();
    all_data data = create_default_data();
    data.wind.i_sin = 0xABCD;
    data.wind.i_cos = 0x5678;
    
    Calibration calib;
    ByteBuffer buf(256);
    make_message(conf, data, calib, buf);
    
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
    configuration conf = create_default_config();
    conf.offset = 123;
    conf.n2k_source = 42;
    conf.angle_smoothing = 75;
    conf.speed_smoothing = 80;
    conf.calibration_score_threshold = 85;
    conf.auto_cal = 1;
    conf.speed_adjustment = 10;
    conf.vane_type = VANE_TYPE_ST60;
    
    all_data data = create_default_data();
    
    Calibration calib;
    ByteBuffer buf(256);
    make_message(conf, data, calib, buf);
    
    // Configuration should be serialized
    TEST_ASSERT_NOT_EQUAL(0, buf.size());
}


// ============================================================================
// Integration Tests
// ============================================================================

/**
 * Test: make_message produces consistent buffer size
 */
void test_make_message_buffer_size(void) {
    configuration conf = create_default_config();
    all_data data = create_default_data();
    data.wind.angle = 45.5;
    data.wind.smooth_angle = 44.2;
    data.wind.ellipse = 1.5;
    data.wind.speed = 15.3;
    data.heap = 50000;
    
    Calibration calib;
    ByteBuffer buf(256);
    make_message(conf, data, calib, buf);
    
    // Buffer should be successfully created and populated
    TEST_ASSERT_TRUE(buf.size() > 0);
}


// ============================================================================
// Correction status (appended after the Wind360 part)
// ============================================================================

static size_t correction_offset(const Calibration &calib)
{
    return 43 + 1 + calib.get_wind360().buffer_size() + 1;
}

void test_make_message_correction_status(void) {
    configuration conf = create_default_config();
    all_data data = create_default_data();
    data.correction.table_id = 0x5330A1DE;
    data.correction.enabled = 1;
    data.correction.corrected = 1;
    data.correction.awa = 38.2193;
    data.correction.aws = 15.1701;

    Calibration calib;
    ByteBuffer buf(256);
    make_message(conf, data, calib, buf);

    size_t off = correction_offset(calib);
    TEST_ASSERT_EQUAL_UINT32(off + 10, buf.length());
    TEST_ASSERT_TRUE(buf.length() <= 128); // BLEWind buffer
    TEST_ASSERT_EQUAL_HEX32(0x5330A1DE, get_uint32(buf, off));
    TEST_ASSERT_EQUAL_UINT8(1, get_uint8(buf, off + 4));
    TEST_ASSERT_EQUAL_UINT8(1, get_uint8(buf, off + 5));
    TEST_ASSERT_EQUAL_UINT16(382, get_uint16(buf, off + 6));
    TEST_ASSERT_EQUAL_UINT16(152, get_uint16(buf, off + 8));
}

void test_make_message_correction_status_nan(void) {
    configuration conf = create_default_config();
    all_data data = create_default_data();

    Calibration calib;
    ByteBuffer buf(256);
    make_message(conf, data, calib, buf);

    size_t off = correction_offset(calib);
    TEST_ASSERT_EQUAL_HEX32(0, get_uint32(buf, off));
    TEST_ASSERT_EQUAL_UINT8(0, get_uint8(buf, off + 4));
    TEST_ASSERT_EQUAL_UINT8(0, get_uint8(buf, off + 5));
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, get_uint16(buf, off + 6));
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, get_uint16(buf, off + 8));
}

void test_make_message_correction_awa_wraps(void) {
    configuration conf = create_default_config();
    all_data data = create_default_data();
    data.correction.awa = 359.97;
    data.correction.aws = 0.0;

    Calibration calib;
    ByteBuffer buf(256);
    make_message(conf, data, calib, buf);

    size_t off = correction_offset(calib);
    TEST_ASSERT_EQUAL_UINT16(0, get_uint16(buf, off + 6));
    TEST_ASSERT_EQUAL_UINT16(0, get_uint16(buf, off + 8));
}

// ============================================================================
// Correction table characteristic (same bytes as the PGN 130900 payload)
// ============================================================================

static correction_table make_t1()
{
    correction_table t;
    correction_clear(t);
    t.enabled = 1;
    t.n_rows = 2;
    t.rows[0] = {60, {{45, -60, 0}, {95, -40, 0}, {150, -20, 0}}};
    t.rows[1] = {100, {{40, -35, 0}, {95, -10, 0}, {150, 20, 5}}};
    return t;
}

static uint32_t le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

void test_make_table_message_t1(void) {
    static const uint8_t T1_PAYLOAD[] = {
        0xFE, 0x9F, 0x01, 0x01, 0xDE, 0xA1, 0x30, 0x53, 0x01, 0x02, 0x3C, 0x00, 0x2D, 0xC4, 0xFF,
        0x00, 0x00, 0x5F, 0xD8, 0xFF, 0x00, 0x00, 0x96, 0xEC, 0xFF, 0x00, 0x00, 0x64, 0x00, 0x28,
        0xDD, 0xFF, 0x00, 0x00, 0x5F, 0xF6, 0xFF, 0x00, 0x00, 0x96, 0x14, 0x00, 0x05, 0x00};
    uint8_t buf[CORRECTION_MAX_PAYLOAD];
    size_t n = make_table_message(make_t1(), buf, sizeof(buf));
    TEST_ASSERT_EQUAL_UINT32(sizeof(T1_PAYLOAD), n);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(T1_PAYLOAD, buf, sizeof(T1_PAYLOAD));
}

// Bytes 4-7 equal the table_id the data characteristic carries, so the app
// can tell a stale or torn read from the current table.
void test_make_table_message_crc_is_table_id(void) {
    correction_table t = make_t1();
    t.enabled = 0;
    uint8_t buf[CORRECTION_MAX_PAYLOAD];
    TEST_ASSERT_TRUE(make_table_message(t, buf, sizeof(buf)) > 0);
    TEST_ASSERT_EQUAL_HEX32(correction_table_id(t), le32(buf + 4));
}

void test_make_table_message_empty(void) {
    correction_table t;
    correction_clear(t);
    uint8_t buf[CORRECTION_MAX_PAYLOAD];
    TEST_ASSERT_EQUAL_UINT32(10, make_table_message(t, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_UINT8(0, buf[8]);
    TEST_ASSERT_EQUAL_UINT8(0, buf[9]);
    TEST_ASSERT_EQUAL_HEX32(0x41D912FF, le32(buf + 4));
}

/**
 * Test: make_message with all fields populated
 */
void test_make_message_full_data(void) {
    configuration conf = create_default_config();
    conf.offset = 50;
    conf.n2k_source = 20;
    conf.angle_smoothing = 60;
    conf.speed_smoothing = 70;
    conf.calibration_score_threshold = 75;
    conf.auto_cal = 1;
    conf.speed_adjustment = 5;
    conf.vane_type = VANE_TYPE_ST60;
    
    all_data data = create_default_data();
    data.wind.angle = 123.45;
    data.wind.smooth_angle = 120.0;
    data.wind.ellipse = 1.8;
    data.wind.speed = 25.7;
    data.heap = 123456;
    data.wind.angle_error = WIND_ERROR_NO_SIGNAL;
    data.wind.i_sin = 2500;
    data.wind.i_cos = 2800;
    
    Calibration calib;
    ByteBuffer buf(256);
    make_message(conf, data, calib, buf);
    
    // Should not crash and produce valid buffer
    TEST_ASSERT_TRUE(buf.size() > 0);
}

// ============================================================================
// Run All Tests
// ============================================================================

void setUp(void) {
    // This is run before each test case
}

void tearDown(void) {
    // This is run after each test case
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    // ========== Angle Encoding Tests ==========
    RUN_TEST(test_make_message_angle_zero);
    RUN_TEST(test_make_message_angle_45);
    RUN_TEST(test_make_message_angle_90);
    RUN_TEST(test_make_message_angle_negative);
    RUN_TEST(test_make_message_angle_359);
    
    // ========== Smooth Angle Tests ==========
    RUN_TEST(test_make_message_smooth_angle);
    
    // ========== Output Angle Tests ==========
    RUN_TEST(test_make_message_output_angle_with_offset);
    RUN_TEST(test_make_message_output_angle_wrap);
    
    // ========== Ellipse Tests ==========
    RUN_TEST(test_make_message_ellipse);
    RUN_TEST(test_make_message_ellipse_2);
    
    // ========== Speed Tests ==========
    RUN_TEST(test_make_message_speed);
    RUN_TEST(test_make_message_speed_nan);
    RUN_TEST(test_make_message_speed_zero);
    
    // ========== Memory & Error Field Tests ==========
    RUN_TEST(test_make_message_memory);
    RUN_TEST(test_make_message_angle_error);
    
    // ========== ADC Reading Tests ==========
    RUN_TEST(test_make_message_adc_readings);
    
    // ========== Configuration Fields Tests ==========
    RUN_TEST(test_make_message_config_fields);
    
    // ========== Integration Tests ==========
    RUN_TEST(test_make_message_buffer_size);
    RUN_TEST(test_make_message_full_data);
    RUN_TEST(test_make_message_correction_status);
    RUN_TEST(test_make_message_correction_status_nan);
    RUN_TEST(test_make_message_correction_awa_wraps);
    RUN_TEST(test_make_table_message_t1);
    RUN_TEST(test_make_table_message_crc_is_table_id);
    RUN_TEST(test_make_table_message_empty);
    
    return UNITY_END();
}