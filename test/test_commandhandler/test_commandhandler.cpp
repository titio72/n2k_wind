#include <unity.h>
#include <string.h>
#include <string>
#include "CommandHandler.h"
#include "DataAndConf.h"
#include "Calibration.h"
#include "Constants.h"
#include "DataAndConf.h"
#include "BLEWind.h"
#include <stdio.h>

// ============================================================================
// Mock Classes
// ============================================================================

/**
 * Mock BLEWind for testing
 */
class MockBLEWind : public IBLEWind
{
public:
    MockBLEWind() : last_device_name(BLE_DEVICE_NAME) {}

    void set_device_name(const char *name) override
    {
        // printf("[BLE] Setting device name to {%s}\n", name);
        last_device_name = name;
    }

    std::string last_device_name;
};

/**
 * Mock Conf for testing (simplified)
 */
class TestConfPersistence : public ConfPersistence
{
public:
    TestConfPersistence() : ConfPersistence() {}

    bool write(configuration &conf) override
    {
        // In test, always succeed
        return true;
    }

    bool read(configuration &conf, int *error_code) override
    {
        if (error_code)
            *error_code = 0; // indicate success
        return true;
    }
};

#define TEST_CONTEXT                     \
    configuration conf;                           \
    TestConfPersistence confPersistence; \
    Calibration calib;                   \
    MockBLEWind ble;                     \
    CommandHandler handler(conf, confPersistence, calib, ble);

// ============================================================================
// Input Validation Tests
// ============================================================================

/**
 * Test: exec_command with NULL input
 */
void test_command_null_input(void)
{
    TEST_CONTEXT
    CommandResult result = handler.exec_command(NULL);
    TEST_ASSERT_EQUAL(CommandResult::MISSING_INPUT, result);
}

/**
 * Test: exec_command with empty string
 */
void test_command_empty_input(void)
{
    TEST_CONTEXT

    CommandResult result = handler.exec_command("");
    TEST_ASSERT_EQUAL(CommandResult::MISSING_INPUT, result);
}

/**
 * Test: exec_command with unknown command
 */
void test_command_unknown(void)
{
    TEST_CONTEXT

    CommandResult result = handler.exec_command("Z");
    TEST_ASSERT_EQUAL(CommandResult::UNKNOWN_COMMAND, result);
}

// ============================================================================
// Heartbeat Command Tests
// ============================================================================

/**
 * Test: H command (heartbeat)
 * Should always succeed
 */
void test_command_heartbeat(void)
{
    TEST_CONTEXT

    CommandResult result = handler.exec_command("H");
    TEST_ASSERT_EQUAL(CommandResult::SUCCESS, result);
}

// ============================================================================
// Speed Adjustment Command Tests
// ============================================================================

/**
 * Test: K command - set speed adjustment to 50
 */
void test_command_speed_adjustment_50(void)
{
    TEST_CONTEXT

    CommandResult result = handler.exec_command("K50");
    TEST_ASSERT_EQUAL(CommandResult::SUCCESS, result);
    TEST_ASSERT_EQUAL_UINT8(50, conf.speed_adjustment);
}

/**
 * Test: K command - set speed adjustment to 0 (min)
 */
void test_command_speed_adjustment_min(void)
{
    TEST_CONTEXT

    CommandResult result = handler.exec_command("K0");
    TEST_ASSERT_EQUAL(CommandResult::SUCCESS, result);
    TEST_ASSERT_EQUAL_UINT8(0, conf.speed_adjustment);
}

/**
 * Test: K command - set speed adjustment to 255 (max)
 */
void test_command_speed_adjustment_max(void)
{
    TEST_CONTEXT

    CommandResult result = handler.exec_command("K255");
    TEST_ASSERT_EQUAL(CommandResult::SUCCESS, result);
    TEST_ASSERT_EQUAL_UINT8(255, conf.speed_adjustment);
}

/**
 * Test: K command - invalid (out of range)
 */
void test_command_speed_adjustment_out_of_range(void)
{
    TEST_CONTEXT

    CommandResult result = handler.exec_command("K256");
    TEST_ASSERT_EQUAL(CommandResult::INVALID_FORMAT, result);
}

/**
 * Test: K command - invalid (non-numeric)
 */
void test_command_speed_adjustment_invalid(void)
{
    TEST_CONTEXT

    CommandResult result = handler.exec_command("Kabc");
    TEST_ASSERT_EQUAL(CommandResult::INVALID_FORMAT, result);
}

// ============================================================================
// Offset Command Tests
// ============================================================================

/**
 * Test: O command - set offset to 45°
 */
void test_command_offset_positive(void)
{
    TEST_CONTEXT

    CommandResult result = handler.exec_command("O45");
    TEST_ASSERT_EQUAL(CommandResult::SUCCESS, result);
    TEST_ASSERT_EQUAL_INT16(45, conf.offset);
}

/**
 * Test: O command - set offset to -90°
 */
void test_command_offset_negative(void)
{
    TEST_CONTEXT

    CommandResult result = handler.exec_command("O-90");
    TEST_ASSERT_EQUAL(CommandResult::SUCCESS, result);
    TEST_ASSERT_EQUAL_INT16(-90, conf.offset);
}

/**
 * Test: O command - set offset to 0°
 */
void test_command_offset_zero(void)
{
    TEST_CONTEXT

    CommandResult result = handler.exec_command("O0");
    TEST_ASSERT_EQUAL(CommandResult::SUCCESS, result);
    TEST_ASSERT_EQUAL_INT16(0, conf.offset);
}

/**
 * Test: O command - set offset to max (360°)
 */
void test_command_offset_max(void)
{
    TEST_CONTEXT

    CommandResult result = handler.exec_command("O360");
    TEST_ASSERT_EQUAL(CommandResult::SUCCESS, result);
    TEST_ASSERT_EQUAL_INT16(360, conf.offset);
}

/**
 * Test: O command - set offset to min (-360°)
 */
void test_command_offset_min(void)
{
    TEST_CONTEXT

    CommandResult result = handler.exec_command("O-360");
    TEST_ASSERT_EQUAL(CommandResult::SUCCESS, result);
    TEST_ASSERT_EQUAL_INT16(-360, conf.offset);
}

/**
 * Test: O command - invalid (out of range)
 */
void test_command_offset_out_of_range(void)
{
    TEST_CONTEXT

    CommandResult result = handler.exec_command("O361");
    TEST_ASSERT_EQUAL(CommandResult::INVALID_FORMAT, result);
}

// ============================================================================
// Vane Type Command Tests
// ============================================================================

/**
 * Test: V command - set vane type to ST50 (0)
 */
void test_command_vane_type_st50(void)
{
    TEST_CONTEXT

    CommandResult result = handler.exec_command("V0");
    TEST_ASSERT_EQUAL(CommandResult::SUCCESS, result);
    TEST_ASSERT_EQUAL_UINT8(VANE_TYPE_ST50, conf.vane_type);
}

/**
 * Test: V command - set vane type to ST60 (1)
 */
void test_command_vane_type_st60(void)
{
    TEST_CONTEXT

    CommandResult result = handler.exec_command("V1");
    TEST_ASSERT_EQUAL(CommandResult::SUCCESS, result);
    TEST_ASSERT_EQUAL_UINT8(VANE_TYPE_ST60, conf.vane_type);
}

/**
 * Test: V command - invalid vane type
 */
void test_command_vane_type_invalid(void)
{
    TEST_CONTEXT

    CommandResult result = handler.exec_command("V2");
    TEST_ASSERT_EQUAL(CommandResult::INVALID_FORMAT, result);
}

// ============================================================================
// Speed Smoothing Command Tests
// ============================================================================

/**
 * Test: W command - set speed smoothing to 50
 */
void test_command_speed_smoothing(void)
{
    TEST_CONTEXT

    CommandResult result = handler.exec_command("W50");
    TEST_ASSERT_EQUAL(CommandResult::SUCCESS, result);
    TEST_ASSERT_EQUAL_UINT8(50, conf.speed_smoothing);
}

/**
 * Test: W command - min value (0)
 */
void test_command_speed_smoothing_min(void)
{
    TEST_CONTEXT

    CommandResult result = handler.exec_command("W0");
    TEST_ASSERT_EQUAL(CommandResult::SUCCESS, result);
    TEST_ASSERT_EQUAL_UINT8(0, conf.speed_smoothing);
}

/**
 * Test: W command - max value (100)
 */
void test_command_speed_smoothing_max(void)
{
    TEST_CONTEXT

    CommandResult result = handler.exec_command("W100");
    TEST_ASSERT_EQUAL(CommandResult::SUCCESS, result);
    TEST_ASSERT_EQUAL_UINT8(100, conf.speed_smoothing);
}

/**
 * Test: W command - out of range
 */
void test_command_speed_smoothing_out_of_range(void)
{
    TEST_CONTEXT

    CommandResult result = handler.exec_command("W101");
    TEST_ASSERT_EQUAL(CommandResult::INVALID_FORMAT, result);
}

// ============================================================================
// Angle Smoothing Command Tests
// ============================================================================

/**
 * Test: Q command - set angle smoothing to 75
 */
void test_command_angle_smoothing(void)
{
    TEST_CONTEXT

    CommandResult result = handler.exec_command("Q75");
    TEST_ASSERT_EQUAL(CommandResult::SUCCESS, result);
    TEST_ASSERT_EQUAL_UINT8(75, conf.angle_smoothing);
}

/**
 * Test: Q command - min value (0)
 */
void test_command_angle_smoothing_min(void)
{
    TEST_CONTEXT

    CommandResult result = handler.exec_command("Q0");
    TEST_ASSERT_EQUAL(CommandResult::SUCCESS, result);
    TEST_ASSERT_EQUAL_UINT8(0, conf.angle_smoothing);
}

/**
 * Test: Q command - max value (100)
 */
void test_command_angle_smoothing_max(void)
{
    TEST_CONTEXT

    CommandResult result = handler.exec_command("Q100");
    TEST_ASSERT_EQUAL(CommandResult::SUCCESS, result);
    TEST_ASSERT_EQUAL_UINT8(100, conf.angle_smoothing);
}

// ============================================================================
// Auto-Calibration Threshold Command Tests
// ============================================================================

/**
 * Test: T command - set threshold to 75
 */
void test_command_auto_calib_threshold(void)
{
    TEST_CONTEXT

    CommandResult result = handler.exec_command("T75");
    TEST_ASSERT_EQUAL(CommandResult::SUCCESS, result);
    TEST_ASSERT_EQUAL_UINT8(75, conf.calibration_score_threshold);
}

/**
 * Test: T command - min value (50)
 */
void test_command_auto_calib_threshold_min(void)
{
    TEST_CONTEXT

    CommandResult result = handler.exec_command("T50");
    TEST_ASSERT_EQUAL(CommandResult::SUCCESS, result);
    TEST_ASSERT_EQUAL_UINT8(50, conf.calibration_score_threshold);
}

/**
 * Test: T command - max value (100)
 */
void test_command_auto_calib_threshold_max(void)
{
    TEST_CONTEXT

    CommandResult result = handler.exec_command("T100");
    TEST_ASSERT_EQUAL(CommandResult::SUCCESS, result);
    TEST_ASSERT_EQUAL_UINT8(100, conf.calibration_score_threshold);
}

/**
 * Test: T command - out of range (too low)
 */
void test_command_auto_calib_threshold_too_low(void)
{
    TEST_CONTEXT

    CommandResult result = handler.exec_command("T49");
    TEST_ASSERT_EQUAL(CommandResult::INVALID_FORMAT, result);
}

/**
 * Test: T command - out of range (too high)
 */
void test_command_auto_calib_threshold_too_high(void)
{
    TEST_CONTEXT

    CommandResult result = handler.exec_command("T101");
    TEST_ASSERT_EQUAL(CommandResult::INVALID_FORMAT, result);
}

// ============================================================================
// BLE Name Command Tests
// ============================================================================

/**
 * Test: N command - change BLE name
 */
void test_command_change_ble_name(void)
{
    TEST_CONTEXT

    CommandResult result = handler.exec_command("NMyWind");
    TEST_ASSERT_EQUAL(CommandResult::SUCCESS, result);
    TEST_ASSERT_EQUAL_STRING("MyWind", conf.ble_name);
    TEST_ASSERT_EQUAL_STRING("MyWind", ble.last_device_name.c_str());
}

/**
 * Test: N command - name too long (overflow)
 */
void test_command_change_ble_name_too_long(void)
{
    TEST_CONTEXT

    // ble_name is 16 chars (15 + null)
    CommandResult result = handler.exec_command("N1234567890123456789");
    TEST_ASSERT_EQUAL(CommandResult::INVALID_FORMAT, result);
}

/**
 * Test: N command - empty name
 */
void test_command_change_ble_name_empty(void)
{
    TEST_CONTEXT

    CommandResult result = handler.exec_command("N");
    TEST_ASSERT_EQUAL(CommandResult::INVALID_FORMAT, result);
}

/**
 * Test: N command - name with spaces
 */
void test_command_change_ble_name_with_spaces(void)
{
    TEST_CONTEXT

    CommandResult result = handler.exec_command("NMy Wind");
    TEST_ASSERT_EQUAL(CommandResult::SUCCESS, result);
    TEST_ASSERT_EQUAL_STRING("My Wind", conf.ble_name);
}

// ============================================================================
// Debug Command Tests
// ============================================================================

/**
 * Test: D command - enable debug (1)
 */
void test_command_debug_enable(void)
{
    TEST_CONTEXT

    CommandResult result = handler.exec_command("D1");
    TEST_ASSERT_EQUAL(CommandResult::SUCCESS, result);
    TEST_ASSERT_EQUAL_UINT8(1, conf.usb_tracing);
}

/**
 * Test: D command - disable debug (0)
 */
void test_command_debug_disable(void)
{
    TEST_CONTEXT

    CommandResult result = handler.exec_command("D0");
    TEST_ASSERT_EQUAL(CommandResult::SUCCESS, result);
    TEST_ASSERT_EQUAL_UINT8(0, conf.usb_tracing);
}

// ============================================================================
// Calibration Command Tests
// ============================================================================

/**
 * Test: A command - abort calibration
 */
void test_command_abort_calibration(void)
{
    TEST_CONTEXT

    CommandResult result = handler.exec_command("A");
    TEST_ASSERT_EQUAL(CommandResult::SUCCESS, result);
}

/**
 * Test: R command - finalize calibration
 */
void test_command_finalize_calibration(void)
{
    TEST_CONTEXT

    CommandResult result = handler.exec_command("R");
    TEST_ASSERT_EQUAL(CommandResult::SUCCESS, result);
}

/**
 * Test: X command - factory reset
 */
void test_command_factory_reset(void)
{
    TEST_CONTEXT
    conf.offset = 45; // Set non-zero offset
    CommandResult result = handler.exec_command("X");
    // Factory reset should clear offset
    TEST_ASSERT_EQUAL_INT16(0, conf.offset);
}

/**
 * Test: P command - toggle auto-calibration
 */
void test_command_toggle_autocalib(void)
{
    TEST_CONTEXT

    // Initial state: auto_cal should be 0
    TEST_ASSERT_EQUAL_UINT8(0, conf.auto_cal);

    CommandResult result = handler.exec_command("P");
    TEST_ASSERT_EQUAL(CommandResult::SUCCESS, result);
    // Should toggle to 1
    TEST_ASSERT_EQUAL_UINT8(1, conf.auto_cal);
}

// ============================================================================
// Manual Calibration Command Tests
// ============================================================================

/**
 * Test: S command - manual calibration with valid values
 */
void test_command_manual_calibration_valid(void)
{
    TEST_CONTEXT

    // S command format: sin_min|sin_max|cos_min|cos_max
    CommandResult result = handler.exec_command("S600|3400|600|3400");
    // Result depends on calibration callback, may be SUCCESS or INTERNAL_ERROR
    TEST_ASSERT_TRUE(result == CommandResult::SUCCESS || result == CommandResult::INTERNAL_ERROR);
}

/**
 * Test: S command - incomplete calibration data
 */
void test_command_manual_calibration_incomplete(void)
{
    TEST_CONTEXT

    CommandResult result = handler.exec_command("S600|3400|600");
    TEST_ASSERT_EQUAL(CommandResult::INVALID_FORMAT, result);
}

/**
 * Test: S command - invalid values
 */
void test_command_manual_calibration_invalid(void)
{
    TEST_CONTEXT

    CommandResult result = handler.exec_command("Sabc|def|ghi|jkl");
    TEST_ASSERT_EQUAL(CommandResult::INVALID_FORMAT, result);
}

// ============================================================================
// Run All Tests
// ============================================================================

void setup(void)
{
    UNITY_BEGIN();

    // Input validation tests
    RUN_TEST(test_command_null_input);
    RUN_TEST(test_command_empty_input);
    RUN_TEST(test_command_unknown);

    // Heartbeat tests
    RUN_TEST(test_command_heartbeat);

    // Speed adjustment tests
    RUN_TEST(test_command_speed_adjustment_50);
    RUN_TEST(test_command_speed_adjustment_min);
    RUN_TEST(test_command_speed_adjustment_max);
    RUN_TEST(test_command_speed_adjustment_out_of_range);
    RUN_TEST(test_command_speed_adjustment_invalid);

    // Offset tests
    RUN_TEST(test_command_offset_positive);
    RUN_TEST(test_command_offset_negative);
    RUN_TEST(test_command_offset_zero);
    RUN_TEST(test_command_offset_max);
    RUN_TEST(test_command_offset_min);
    RUN_TEST(test_command_offset_out_of_range);

    // Vane type tests
    RUN_TEST(test_command_vane_type_st50);
    RUN_TEST(test_command_vane_type_st60);
    RUN_TEST(test_command_vane_type_invalid);

    // Speed smoothing tests
    RUN_TEST(test_command_speed_smoothing);
    RUN_TEST(test_command_speed_smoothing_min);
    RUN_TEST(test_command_speed_smoothing_max);
    RUN_TEST(test_command_speed_smoothing_out_of_range);

    // Angle smoothing tests
    RUN_TEST(test_command_angle_smoothing);
    RUN_TEST(test_command_angle_smoothing_min);
    RUN_TEST(test_command_angle_smoothing_max);

    // Auto-calib threshold tests
    RUN_TEST(test_command_auto_calib_threshold);
    RUN_TEST(test_command_auto_calib_threshold_min);
    RUN_TEST(test_command_auto_calib_threshold_max);
    RUN_TEST(test_command_auto_calib_threshold_too_low);
    RUN_TEST(test_command_auto_calib_threshold_too_high);

    // BLE name tests
    RUN_TEST(test_command_change_ble_name);
    RUN_TEST(test_command_change_ble_name_too_long);
    RUN_TEST(test_command_change_ble_name_empty);
    RUN_TEST(test_command_change_ble_name_with_spaces);

    // Debug tests
    RUN_TEST(test_command_debug_enable);
    RUN_TEST(test_command_debug_disable);

    // Calibration tests
    RUN_TEST(test_command_abort_calibration);
    RUN_TEST(test_command_finalize_calibration);
    RUN_TEST(test_command_factory_reset);
    RUN_TEST(test_command_toggle_autocalib);

    // Manual calibration tests
    RUN_TEST(test_command_manual_calibration_valid);
    RUN_TEST(test_command_manual_calibration_incomplete);
    RUN_TEST(test_command_manual_calibration_invalid);

    UNITY_END();
}

void loop(void)
{
    // Empty
}

int main(int argc, char **argv)
{
    setup();
    return 0;
}