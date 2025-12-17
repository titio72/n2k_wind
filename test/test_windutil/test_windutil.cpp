#include <unity.h>
#include <math.h>
#include <limits.h>
#include "../src/WindUtil.h"

// ============================================================================
// Range Class Tests
// ============================================================================

/**
 * Test: Range default construction
 * Default range uses RANGE_DEFAULT_MIN, RANGE_DEFAULT_MAX, RANGE_DEFAULT_VALID
 */
void test_range_default_construction(void) {
    Range r;
    TEST_ASSERT_EQUAL_UINT16(RANGE_DEFAULT_MIN, r.low());
    TEST_ASSERT_EQUAL_UINT16(RANGE_DEFAULT_MAX, r.high());
    TEST_ASSERT_TRUE(r.is_valid()); // Default range should be valid
}

/**
 * Test: Range custom construction
 */
void test_range_custom_construction(void) {
    Range r(100, 200, 50);
    TEST_ASSERT_EQUAL_UINT16(100, r.low());
    TEST_ASSERT_EQUAL_UINT16(200, r.high());
    TEST_ASSERT_EQUAL_UINT16(100, r.range());
    TEST_ASSERT_TRUE(r.is_valid());  // 100 > 50
}

/**
 * Test: Range validation
 * is_valid() checks if range exceeds minimum_valid_span
 */
void test_range_is_valid(void) {
    // Valid: range >= min_span
    Range valid(0, 100, 50);
    TEST_ASSERT_TRUE(valid.is_valid());  // range=100 > 50

    // Invalid: range <= min_span
    Range invalid(0, 50, 100);
    TEST_ASSERT_FALSE(invalid.is_valid());  // range=50 < 100

    // Edge: range exactly equals min_span
    Range edge(0, 50, 50);
    TEST_ASSERT_FALSE(edge.is_valid());  // range=50 is NOT > 50
}

/**
 * Test: Range set with two uint16_t
 */
void test_range_set_values(void) {
    Range r;
    r.set(500, 1000);
    TEST_ASSERT_EQUAL_UINT16(500, r.low());
    TEST_ASSERT_EQUAL_UINT16(1000, r.high());
    TEST_ASSERT_EQUAL_UINT16(500, r.range());
}

/**
 * Test: Range set with another Range
 */
void test_range_set_range(void) {
    Range r1(100, 200, 50);
    Range r2;
    r2.set(r1);
    
    TEST_ASSERT_EQUAL_UINT16(r1.low(), r2.low());
    TEST_ASSERT_EQUAL_UINT16(r1.high(), r2.high());
    TEST_ASSERT_EQUAL_UINT16(r1.range(), r2.range());
}

/**
 * Test: Range assignment operator
 */
void test_range_assignment(void) {
    Range r1(100, 300, 100);
    Range r2;
    r2 = r1;
    
    TEST_ASSERT_EQUAL_UINT16(100, r2.low());
    TEST_ASSERT_EQUAL_UINT16(300, r2.high());
}

/**
 * Test: Range expand method
 * Expand should grow min/max to include new sample
 */
void test_range_expand(void) {
    Range r(100, 200);
    
    // Expand lower bound
    r.expand(50);
    TEST_ASSERT_EQUAL_UINT16(50, r.low());
    TEST_ASSERT_EQUAL_UINT16(200, r.high());
    
    // Expand upper bound
    r.expand(300);
    TEST_ASSERT_EQUAL_UINT16(50, r.low());
    TEST_ASSERT_EQUAL_UINT16(300, r.high());
    
    // Value within range (no change)
    r.expand(150);
    TEST_ASSERT_EQUAL_UINT16(50, r.low());
    TEST_ASSERT_EQUAL_UINT16(300, r.high());
}

/**
 * Test: Range to_analog conversion
 * Maps ADC reading to analog value using range
 */
void test_range_to_analog(void) {
    Range r(0, 1000, 100);  // ADC range
    
    // Map to [-1, 1]
    double mid = r.to_analog(-1.0, 1.0, 500);  // Middle of ADC range
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.0, mid);
    
    double low = r.to_analog(-1.0, 1.0, 0);    // Bottom of ADC range
    TEST_ASSERT_DOUBLE_WITHIN(0.01, -1.0, low);
    
    double high = r.to_analog(-1.0, 1.0, 1000); // Top of ADC range
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 1.0, high);
}

/**
 * Test: Range to_analog with invalid range
 * Should return NAN if range is not valid
 */
void test_range_to_analog_invalid(void) {
    Range r(0, 100, 1000);  // invalid range
    double result = r.to_analog(-1.0, 1.0, 500);
    TEST_ASSERT_TRUE(isnan(result));
}

/**
 * Test: Range to_analog with different output ranges
 */
void test_range_to_analog_output_ranges(void) {
    Range r(0, 100, 50);
    
    // Map to [0, 10]
    double val = r.to_analog(0.0, 10.0, 50);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 5.0, val);
    
    // Map to [-100, 100]
    val = r.to_analog(-100.0, 100.0, 25);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, -50.0, val);
}

// ============================================================================
// Angle Normalization Tests
// ============================================================================

/**
 * Test: norm_deg with double
 * Normalize angle to [0, 360)
 */
void test_norm_deg_double(void) {
    // Positive angles
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 45.0, norm_deg(45.0));
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.0, norm_deg(360.0));
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.0, norm_deg(720.0));
    
    // Negative angles
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 315.0, norm_deg(-45.0));
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 180.0, norm_deg(-180.0));
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.0, norm_deg(-360.0));
    
    // Edge cases
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.0, norm_deg(0.0));
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 359.9, norm_deg(359.9));
}

/**
 * Test: norm_deg with int16_t
 * Normalize angle to [0, 360)
 */
void test_norm_deg_int16(void) {
    // Positive angles
    TEST_ASSERT_EQUAL_INT16(45, norm_deg((int16_t)45));
    TEST_ASSERT_EQUAL_INT16(0, norm_deg((int16_t)360));
    TEST_ASSERT_EQUAL_INT16(0, norm_deg((int16_t)720));
    
    // Negative angles
    TEST_ASSERT_EQUAL_INT16(315, norm_deg((int16_t)-45));
    TEST_ASSERT_EQUAL_INT16(180, norm_deg((int16_t)-180));
    TEST_ASSERT_EQUAL_INT16(0, norm_deg((int16_t)-360));
}

/**
 * Test: norm_deg with very large angles
 */
void test_norm_deg_large_angles(void) {
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 45.0, norm_deg(3600.0 + 45.0));  // 10 full rotations
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 270.0, norm_deg(7200.0 + 270.0));  // 20 full rotations
}

// ============================================================================
// String Tokenization Tests
// ============================================================================

/**
 * Test: mystrtok basic splitting
 * Split string by delimiter character
 */
void test_mystrtok_basic(void) {
    char s[] = "one|two|three";
    char *m = s;
    
    char *tok1 = mystrtok(&m, s, '|');
    TEST_ASSERT_EQUAL_STRING("one", tok1);
    
    char *tok2 = mystrtok(&m, NULL, '|');
    TEST_ASSERT_EQUAL_STRING("two", tok2);
    
    char *tok3 = mystrtok(&m, NULL, '|');
    TEST_ASSERT_EQUAL_STRING("three", tok3);
    
    char *tok4 = mystrtok(&m, NULL, '|');
    TEST_ASSERT_NULL(tok4);  // End of string
}

/**
 * Test: mystrtok with no delimiter
 * String without delimiter should return whole string
 */
void test_mystrtok_no_delimiter(void) {
    char s[] = "nodots";
    char *m = s;
    
    char *tok = mystrtok(&m, s, '.');
    TEST_ASSERT_EQUAL_STRING("nodots", tok);
    
    char *tok2 = mystrtok(&m, NULL, '.');
    TEST_ASSERT_NULL(tok2);
}

/**
 * Test: mystrtok empty tokens
 * Adjacent delimiters create empty tokens
 */
void test_mystrtok_empty_tokens(void) {
    char s[] = "a||b";
    char *m = s;
    
    char *tok1 = mystrtok(&m, s, '|');
    TEST_ASSERT_EQUAL_STRING("a", tok1);
    
    char *tok2 = mystrtok(&m, NULL, '|');
    TEST_ASSERT_EQUAL_STRING("", tok2);  // Empty token
    
    char *tok3 = mystrtok(&m, NULL, '|');
    TEST_ASSERT_EQUAL_STRING("b", tok3);
}

// ============================================================================
// String to Integer Conversion Tests
// ============================================================================

/**
 * Test: atoi_x basic conversion
 */
void test_atoi_x_basic(void) {
    int32_t value = 0;
    
    TEST_ASSERT_TRUE(atoi_x(value, "123"));
    TEST_ASSERT_EQUAL_INT32(123, value);
    
    TEST_ASSERT_TRUE(atoi_x(value, "0"));
    TEST_ASSERT_EQUAL_INT32(0, value);
    
    TEST_ASSERT_TRUE(atoi_x(value, "9999"));
    TEST_ASSERT_EQUAL_INT32(9999, value);
}

/**
 * Test: atoi_x negative numbers
 */
void test_atoi_x_negative(void) {
    int32_t value = 0;
    
    TEST_ASSERT_TRUE(atoi_x(value, "-123"));
    TEST_ASSERT_EQUAL_INT32(-123, value);
    
    TEST_ASSERT_TRUE(atoi_x(value, "-1"));
    TEST_ASSERT_EQUAL_INT32(-1, value);
}

/**
 * Test: atoi_x invalid input
 */
void test_atoi_x_invalid(void) {
    int32_t value = 0;
    
    TEST_ASSERT_FALSE(atoi_x(value, "abc"));
    TEST_ASSERT_FALSE(atoi_x(value, "12a"));
    TEST_ASSERT_FALSE(atoi_x(value, ""));
    TEST_ASSERT_FALSE(atoi_x(value, NULL));
}

/**
 * Test: atoi_x with leading zeros
 */
void test_atoi_x_leading_zeros(void) {
    int32_t value = 0;
    
    TEST_ASSERT_TRUE(atoi_x(value, "0001"));
    TEST_ASSERT_EQUAL_INT32(1, value);
    
    TEST_ASSERT_TRUE(atoi_x(value, "00100"));
    TEST_ASSERT_EQUAL_INT32(100, value);
}

// ============================================================================
// Parse Value Tests
// ============================================================================

/**
 * Test: parse_value basic conversion within range
 */
void test_parse_value_valid(void) {
    int32_t value = 0;
    
    TEST_ASSERT_TRUE(parse_value(value, "50", 100, 0));
    TEST_ASSERT_EQUAL_INT32(50, value);
    
    TEST_ASSERT_TRUE(parse_value(value, "0", 100, 0));
    TEST_ASSERT_EQUAL_INT32(0, value);
    
    TEST_ASSERT_TRUE(parse_value(value, "100", 100, 0));
    TEST_ASSERT_EQUAL_INT32(100, value);
}

/**
 * Test: parse_value out of range
 */
void test_parse_value_out_of_range(void) {
    int32_t value = 999;
    
    TEST_ASSERT_FALSE(parse_value(value, "101", 100, 0));  // Above max
    TEST_ASSERT_EQUAL_INT32(999, value);  // Unchanged
    
    TEST_ASSERT_FALSE(parse_value(value, "-1", 100, 0));   // Below min
    TEST_ASSERT_EQUAL_INT32(999, value);  // Unchanged
}

/**
 * Test: parse_value with negative ranges
 */
void test_parse_value_negative_range(void) {
    int32_t value = 0;
    
    TEST_ASSERT_TRUE(parse_value(value, "-50", 0, -100));
    TEST_ASSERT_EQUAL_INT32(-50, value);
    
    TEST_ASSERT_FALSE(parse_value(value, "-101", 0, -100));
}

/**
 * Test: parse_value invalid input
 */
void test_parse_value_invalid(void) {
    int32_t value = 999;
    
    TEST_ASSERT_FALSE(parse_value(value, "abc", 100, 0));
    TEST_ASSERT_FALSE(parse_value(value, NULL, 100, 0));
    TEST_ASSERT_FALSE(parse_value(value, "", 100, 0));
}

// ============================================================================
// Low-Pass Filter Angle Tests
// ============================================================================

/**
 * Test: lpf_angle basic filtering
 * Should smoothly transition between angles
 */
void test_lpf_angle_basic(void) {
    double current = 0.0;
    double filtered = current;
    
    // Alpha=0.5: average between old and new
    filtered = lpf_angle(filtered, 10.0, 0.5);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 5.0, filtered);  // (0 + 10) * 0.5
    
    filtered = lpf_angle(filtered, 10.0, 0.5);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 7.5, filtered);  // (5 + 10) * 0.5
}

/**
 * Test: lpf_angle with NAN previous (first sample)
 * Should return current value as-is
 */
void test_lpf_angle_nan_previous(void) {
    double result = lpf_angle(NAN, 45.0, 0.5);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 45.0, result);
}

/**
 * Test: lpf_angle with NAN current
 * Should return previous value as-is
 */
void test_lpf_angle_nan_current(void) {
    double result = lpf_angle(45.0, NAN, 0.5);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 45.0, result);
}

/**
 * Test: lpf_angle wrapping across 0°
 * Should take shortest path around 360° boundary
 */
void test_lpf_angle_wrap_positive(void) {
    // Jump from 350° to 10° (should go forward +20°, not backward -340°)
    double result = lpf_angle(350.0, 10.0, 0.5);
    // Diff = 10 - 350 = -340, wraps to +20
    // Result = 350 + 0.5 * 20 = 360 = 0°
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.0, result);
}

/**
 * Test: lpf_angle wrapping across 0° (other direction)
 */
void test_lpf_angle_wrap_negative(void) {
    // Jump from 10° to 350° (should go backward -20°, not forward +340°)
    double result = lpf_angle(10.0, 350.0, 0.5);
    // Diff = 350 - 10 = 340, wraps to -20
    // Result = 10 + 0.5 * (-20) = 0°
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.0, result);
}

/**
 * Test: lpf_angle with alpha=0 (no filtering)
 * Should return previous value unchanged
 */
void test_lpf_angle_alpha_zero(void) {
    double result = lpf_angle(45.0, 90.0, 0.0);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 45.0, result);
}

/**
 * Test: lpf_angle with alpha=1 (full update)
 * Should return current value
 */
void test_lpf_angle_alpha_one(void) {
    double result = lpf_angle(45.0, 90.0, 1.0);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 90.0, result);
}

/**
 * Test: lpf_angle with various alpha values
 * Should blend proportionally
 */
void test_lpf_angle_alpha_blend(void) {
    // alpha=0.1: mostly previous, little new
    double result1 = lpf_angle(0.0, 100.0, 0.1);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 10.0, result1);
    
    // alpha=0.9: mostly new, little previous
    double result2 = lpf_angle(0.0, 100.0, 0.9);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 90.0, result2);
}

// ============================================================================
// Error Flag Tests
// ============================================================================

/**
 * Test: set_error flag setting
 */
void test_set_error_set_flag(void) {
    uint8_t error = 0;
    set_error(error, true, 0x01);
    TEST_ASSERT_EQUAL_UINT8(0x01, error);
}

/**
 * Test: set_error flag clearing
 */
void test_set_error_clear_flag(void) {
    uint8_t error = 0x01;
    set_error(error, false, 0x01);
    TEST_ASSERT_EQUAL_UINT8(0x00, error);
}

/**
 * Test: set_error multiple flags
 */
void test_set_error_multiple_flags(void) {
    uint8_t error = 0;
    
    set_error(error, true, 0x01);  // Set bit 0
    TEST_ASSERT_EQUAL_UINT8(0x01, error);
    
    set_error(error, true, 0x04);  // Set bit 2
    TEST_ASSERT_EQUAL_UINT8(0x05, error);
    
    set_error(error, false, 0x01); // Clear bit 0
    TEST_ASSERT_EQUAL_UINT8(0x04, error);
}

/**
 * Test: set_error no interference
 * Clearing one flag shouldn't affect others
 */
void test_set_error_no_interference(void) {
    uint8_t error = 0xFF;  // All bits set
    
    set_error(error, false, 0x01);  // Clear only bit 0
    TEST_ASSERT_EQUAL_UINT8(0xFE, error);
}

// ============================================================================
// Run All Tests
// ============================================================================

void setup(void) {
    UNITY_BEGIN();
    
    // Range tests
    RUN_TEST(test_range_default_construction);
    RUN_TEST(test_range_custom_construction);
    RUN_TEST(test_range_is_valid);
    RUN_TEST(test_range_set_values);
    RUN_TEST(test_range_set_range);
    RUN_TEST(test_range_assignment);
    RUN_TEST(test_range_expand);
    RUN_TEST(test_range_to_analog);
    RUN_TEST(test_range_to_analog_invalid);
    RUN_TEST(test_range_to_analog_output_ranges);
    
    // Angle normalization tests
    RUN_TEST(test_norm_deg_double);
    RUN_TEST(test_norm_deg_int16);
    RUN_TEST(test_norm_deg_large_angles);
    
    // String tokenization tests
    RUN_TEST(test_mystrtok_basic);
    RUN_TEST(test_mystrtok_no_delimiter);
    RUN_TEST(test_mystrtok_empty_tokens);
    
    // String to int conversion tests
    RUN_TEST(test_atoi_x_basic);
    RUN_TEST(test_atoi_x_negative);
    RUN_TEST(test_atoi_x_invalid);
    RUN_TEST(test_atoi_x_leading_zeros);
    
    // Parse value tests
    RUN_TEST(test_parse_value_valid);
    RUN_TEST(test_parse_value_out_of_range);
    RUN_TEST(test_parse_value_negative_range);
    RUN_TEST(test_parse_value_invalid);
    
    // LPF angle tests
    RUN_TEST(test_lpf_angle_basic);
    RUN_TEST(test_lpf_angle_nan_previous);
    RUN_TEST(test_lpf_angle_nan_current);
    RUN_TEST(test_lpf_angle_wrap_positive);
    RUN_TEST(test_lpf_angle_wrap_negative);
    RUN_TEST(test_lpf_angle_alpha_zero);
    RUN_TEST(test_lpf_angle_alpha_one);
    RUN_TEST(test_lpf_angle_alpha_blend);
    
    // Error flag tests
    RUN_TEST(test_set_error_set_flag);
    RUN_TEST(test_set_error_clear_flag);
    RUN_TEST(test_set_error_multiple_flags);
    RUN_TEST(test_set_error_no_interference);
    
    UNITY_END();
}

void loop(void) {
    // Empty
}

int main(int argc, char **argv) {
    setup();
    return 0;
}