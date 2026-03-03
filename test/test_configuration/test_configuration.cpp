#include <unity.h>
#include <string.h>
#include <math.h>
#include "DataAndConf.h"
#include "Constants.h"
#include "WindUtil.h"
#include "MockEEPROM.h"

// ============================================================================
// configuration - Default Construction / reset()
// ============================================================================

/**
 * Test: Default constructor initialises serial to CONF_SERIAL
 */
void test_default_serial(void) {
    configuration conf;
    TEST_ASSERT_EQUAL(CONF_SERIAL, conf.serial);
}

/**
 * Test: Default constructor sets sin_range to [RANGE_DEFAULT_MIN, RANGE_DEFAULT_MAX]
 */
void test_default_sin_range(void) {
    configuration conf;
    TEST_ASSERT_EQUAL_UINT16(RANGE_DEFAULT_MIN, conf.sin_range.low());
    TEST_ASSERT_EQUAL_UINT16(RANGE_DEFAULT_MAX, conf.sin_range.high());
}

/**
 * Test: Default constructor sets cos_range to [RANGE_DEFAULT_MIN, RANGE_DEFAULT_MAX]
 */
void test_default_cos_range(void) {
    configuration conf;
    TEST_ASSERT_EQUAL_UINT16(RANGE_DEFAULT_MIN, conf.cos_range.low());
    TEST_ASSERT_EQUAL_UINT16(RANGE_DEFAULT_MAX, conf.cos_range.high());
}

/**
 * Test: Default constructor sets offset to 0
 */
void test_default_offset(void) {
    configuration conf;
    TEST_ASSERT_EQUAL_INT16(0, conf.offset);
}

/**
 * Test: Default constructor sets speed_smoothing to DEFAULT_WIND_SPEED_SMOOTHING
 */
void test_default_speed_smoothing(void) {
    configuration conf;
    TEST_ASSERT_EQUAL_UINT8(DEFAULT_WIND_SPEED_SMOOTHING, conf.speed_smoothing);
}

/**
 * Test: Default constructor sets angle_smoothing to DEFAULT_WIND_ANGLE_SMOOTHING
 */
void test_default_angle_smoothing(void) {
    configuration conf;
    TEST_ASSERT_EQUAL_UINT8(DEFAULT_WIND_ANGLE_SMOOTHING, conf.angle_smoothing);
}

/**
 * Test: Default constructor sets speed_adjustment to 100
 */
void test_default_speed_adjustment(void) {
    configuration conf;
    TEST_ASSERT_EQUAL_UINT8(100, conf.speed_adjustment);
}

/**
 * Test: Default constructor sets n2k_source to DEFAULT_N2K_SOURCE
 */
void test_default_n2k_source(void) {
    configuration conf;
    TEST_ASSERT_EQUAL_UINT8(DEFAULT_N2K_SOURCE, conf.n2k_source);
}

/**
 * Test: Default constructor sets auto_cal to 0 (disabled)
 */
void test_default_auto_cal(void) {
    configuration conf;
    TEST_ASSERT_EQUAL_UINT8(0, conf.auto_cal);
}

/**
 * Test: Default constructor sets calibration_score_threshold to AUTO_CALIBRATION_SCORE_THRESHOLD_DEFAULT * 100
 */
void test_default_calibration_score_threshold(void) {
    configuration conf;
    uint8_t expected = (uint8_t)(AUTO_CALIBRATION_SCORE_THRESHOLD_DEFAULT * 100);
    TEST_ASSERT_EQUAL_UINT8(expected, conf.calibration_score_threshold);
}

/**
 * Test: Default constructor sets usb_tracing to 1 (enabled)
 */
void test_default_usb_tracing(void) {
    configuration conf;
    TEST_ASSERT_EQUAL_UINT8(1, conf.usb_tracing);
}

/**
 * Test: Default constructor sets vane_type to VANE_TYPE_DEFAULT
 */
void test_default_vane_type(void) {
    configuration conf;
    TEST_ASSERT_EQUAL_UINT8(VANE_TYPE_DEFAULT, conf.vane_type);
}

/**
 * Test: Default constructor sets ble_name to BLE_DEVICE_NAME
 */
void test_default_ble_name(void) {
    configuration conf;
    TEST_ASSERT_EQUAL_STRING(BLE_DEVICE_NAME, conf.ble_name);
}

/**
 * Test: reset() restores modified fields back to defaults
 */
void test_reset_restores_defaults(void) {
    configuration conf;
    conf.offset = 45;
    conf.speed_smoothing = 99;
    conf.angle_smoothing = 10;
    conf.auto_cal = 1;
    conf.reset();

    TEST_ASSERT_EQUAL(CONF_SERIAL, conf.serial);
    TEST_ASSERT_EQUAL_INT16(0, conf.offset);
    TEST_ASSERT_EQUAL_UINT8(DEFAULT_WIND_SPEED_SMOOTHING, conf.speed_smoothing);
    TEST_ASSERT_EQUAL_UINT8(DEFAULT_WIND_ANGLE_SMOOTHING, conf.angle_smoothing);
    TEST_ASSERT_EQUAL_UINT8(0, conf.auto_cal);
    TEST_ASSERT_EQUAL_STRING(BLE_DEVICE_NAME, conf.ble_name);
}

// ============================================================================
// configuration - Getter scaling methods
// ============================================================================

/**
 * Test: get_speed_smoothing_factor() returns speed_smoothing / 100.0
 */
void test_get_speed_smoothing_factor(void) {
    configuration conf;
    conf.speed_smoothing = 50;
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.50, conf.get_speed_smoothing_factor());

    conf.speed_smoothing = 0;
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.00, conf.get_speed_smoothing_factor());

    conf.speed_smoothing = 100;
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 1.00, conf.get_speed_smoothing_factor());
}

/**
 * Test: get_angle_smoothing_factor() returns angle_smoothing / 100.0
 */
void test_get_angle_smoothing_factor(void) {
    configuration conf;
    conf.angle_smoothing = 75;
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.75, conf.get_angle_smoothing_factor());

    conf.angle_smoothing = 0;
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.00, conf.get_angle_smoothing_factor());

    conf.angle_smoothing = 100;
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 1.00, conf.get_angle_smoothing_factor());
}

/**
 * Test: get_speed_adjustement() returns speed_adjustment / 100.0
 */
void test_get_speed_adjustment_factor(void) {
    configuration conf;
    conf.speed_adjustment = 100;
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 1.00, conf.get_speed_adjustement());

    conf.speed_adjustment = 50;
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.50, conf.get_speed_adjustement());

    conf.speed_adjustment = 0;
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.00, conf.get_speed_adjustement());
}

/**
 * Test: get_calibration_threshold_factor() returns calibration_score_threshold / 100.0
 */
void test_get_calibration_threshold_factor(void) {
    configuration conf;
    conf.calibration_score_threshold = 75;
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.75, conf.get_calibration_threshold_factor());

    conf.calibration_score_threshold = 50;
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.50, conf.get_calibration_threshold_factor());

    conf.calibration_score_threshold = 100;
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 1.00, conf.get_calibration_threshold_factor());
}

// ============================================================================
// configuration - set_ble_name()
// ============================================================================

/**
 * Test: set_ble_name() copies name into ble_name
 */
void test_set_ble_name_basic(void) {
    configuration conf;
    conf.set_ble_name("TestDevice");
    TEST_ASSERT_EQUAL_STRING("TestDevice", conf.ble_name);
}

/**
 * Test: set_ble_name() with null pointer does not crash and keeps null terminator
 */
void test_set_ble_name_null(void) {
    configuration conf;
    conf.set_ble_name("Before");
    conf.set_ble_name(nullptr);
    // ble_name[last] must still be null-terminated
    TEST_ASSERT_EQUAL('\0', conf.ble_name[sizeof(conf.ble_name) - 1]);
}

/**
 * Test: set_ble_name() truncates names longer than buffer (15 chars + null)
 */
void test_set_ble_name_truncation(void) {
    configuration conf;
    // ble_name is char[16]: 15 visible chars + '\0'
    conf.set_ble_name("1234567890123456789"); // 19 chars, longer than buffer
    TEST_ASSERT_EQUAL('\0', conf.ble_name[sizeof(conf.ble_name) - 1]);
    // Length must be at most 15
    TEST_ASSERT_TRUE(strlen(conf.ble_name) <= sizeof(conf.ble_name) - 1);
}

/**
 * Test: set_ble_name() with empty string
 */
void test_set_ble_name_empty(void) {
    configuration conf;
    conf.set_ble_name("");
    TEST_ASSERT_EQUAL_STRING("", conf.ble_name);
}

// ============================================================================
// all_data - get_out_angle()
// ============================================================================

/**
 * Test: get_out_angle() returns smooth_angle + offset, normalised to [0, 360)
 */
void test_get_out_angle_no_offset(void) {
    all_data data;
    configuration conf;
    conf.offset = 0;
    data.wind.smooth_angle = 90.0;
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 90.0, data.get_out_angle(conf));
}

/**
 * Test: get_out_angle() applies positive offset correctly
 */
void test_get_out_angle_positive_offset(void) {
    all_data data;
    configuration conf;
    conf.offset = 30;
    data.wind.smooth_angle = 90.0;
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 120.0, data.get_out_angle(conf));
}

/**
 * Test: get_out_angle() applies negative offset and wraps below 0
 */
void test_get_out_angle_wrap_below_zero(void) {
    all_data data;
    configuration conf;
    conf.offset = -30;
    data.wind.smooth_angle = 10.0;
    // 10 - 30 = -20 -> normalised -> 340
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 340.0, data.get_out_angle(conf));
}

/**
 * Test: get_out_angle() wraps above 360
 */
void test_get_out_angle_wrap_above_360(void) {
    all_data data;
    configuration conf;
    conf.offset = 40;
    data.wind.smooth_angle = 340.0;
    // 340 + 40 = 380 -> normalised -> 20
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 20.0, data.get_out_angle(conf));
}


void test_save_and_read() {
    configuration conf;
    conf.offset = 45;
    conf.speed_smoothing = 99;
    conf.angle_smoothing = 10;
    conf.auto_cal = 1;
    conf.usb_tracing = 1;
    conf.vane_type = VANE_TYPE_ST60;
    conf.n2k_source = 42;
    conf.set_ble_name("TestDevice");

    #ifdef NATIVE
    printf("Running EEPROM save/read test with MockEEPROM\n");
    #else
    printf("Running EEPROM save/read test with actual EEPROM\n");
    #endif


    ConfPersistence persistence;
    printf("Testing configuration save and read... size {%d}\n", sizeof(configuration));
    persistence.write(conf);
    //TEST_ASSERT(persistence.write(conf));
    //mockEEPROM.begin(sizeof(configuration));
    //mockEEPROM.put(0, conf);
    //mockEEPROM.commit();
    uint8_t* eeprom_data = (uint8_t*)&conf;

    printf("Configuration written to EEPROM {%d %d}\n", eeprom_data[0], mockEEPROM.read(0)); // Debug: print first 2 bytes of EEPROM (serial)
    TEST_ASSERT_EQUAL(1, (int)mockEEPROM.get_write_count()); // Expect exactly 1 write operation for the whole struct
    TEST_ASSERT_EQUAL(1, (int)mockEEPROM.get_commit_count()); // Expect exactly 1 write operation for the whole struct

    configuration conf_read;
    int error_code = 0;
    persistence.read(conf_read, &error_code);
    TEST_ASSERT_EQUAL(0, error_code);
    TEST_ASSERT_EQUAL(conf.serial, conf_read.serial);
    TEST_ASSERT_EQUAL(conf.offset, conf_read.offset);
    TEST_ASSERT_EQUAL(conf.speed_smoothing, conf_read.speed_smoothing);
    TEST_ASSERT_EQUAL(conf.angle_smoothing, conf_read.angle_smoothing);
    TEST_ASSERT_EQUAL(conf.auto_cal, conf_read.auto_cal);
    TEST_ASSERT_EQUAL_STRING(conf.ble_name, conf_read.ble_name);
    TEST_ASSERT_EQUAL(conf.n2k_source, conf_read.n2k_source);
    TEST_ASSERT_EQUAL(conf.vane_type, conf_read.vane_type);
    TEST_ASSERT_EQUAL(conf.usb_tracing, conf_read.usb_tracing);
}

// ============================================================================
// Main
// ============================================================================

void setup(void) {
    UNITY_BEGIN();

    // Default construction / reset
    RUN_TEST(test_default_serial);
    RUN_TEST(test_default_sin_range);
    RUN_TEST(test_default_cos_range);
    RUN_TEST(test_default_offset);
    RUN_TEST(test_default_speed_smoothing);
    RUN_TEST(test_default_angle_smoothing);
    RUN_TEST(test_default_speed_adjustment);
    RUN_TEST(test_default_n2k_source);
    RUN_TEST(test_default_auto_cal);
    RUN_TEST(test_default_calibration_score_threshold);
    RUN_TEST(test_default_usb_tracing);
    RUN_TEST(test_default_vane_type);
    RUN_TEST(test_default_ble_name);
    RUN_TEST(test_reset_restores_defaults);

    // Getter scaling methods
    RUN_TEST(test_get_speed_smoothing_factor);
    RUN_TEST(test_get_angle_smoothing_factor);
    RUN_TEST(test_get_speed_adjustment_factor);
    RUN_TEST(test_get_calibration_threshold_factor);

    // set_ble_name
    RUN_TEST(test_set_ble_name_basic);
    RUN_TEST(test_set_ble_name_null);
    RUN_TEST(test_set_ble_name_truncation);
    RUN_TEST(test_set_ble_name_empty);

    // all_data::get_out_angle
    RUN_TEST(test_get_out_angle_no_offset);
    RUN_TEST(test_get_out_angle_positive_offset);
    RUN_TEST(test_get_out_angle_wrap_below_zero);
    RUN_TEST(test_get_out_angle_wrap_above_360);

    // save and read
    RUN_TEST(test_save_and_read);

    UNITY_END();
}

void loop(void) {
    // Empty
}

int main(int argc, char **argv) {
    setup();
    return 0;
}
