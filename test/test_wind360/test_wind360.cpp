#include <unity.h>
#include <math.h>
#include <stdio.h>
#include "../src/Wind360.h"
#include "../src/WindUtil.h"

// Helper to print uint8_t in binary (for debugging)
void print_byte_binary(uint8_t b) {
    for (int i = 7; i >= 0; i--) {
        printf("%d", (b >> i) & 1);
    }
}

/**
 * Test: Construction with default size
 */
void test_wind360_construction_default(void) {
    Wind360 w;
    TEST_ASSERT_EQUAL_INT(90, w.size());  // Default WIND360_SIZE = 90
    TEST_ASSERT_EQUAL_INT(0, w.progress());
    TEST_ASSERT_EQUAL_DOUBLE(0.0, w.get_score());
    TEST_ASSERT_FALSE(w.is_valid());
}

/**
 * Test: Construction with custom size
 */
void test_wind360_construction_custom(void) {
    Wind360 w(8);  // 8 buckets = 45° each
    TEST_ASSERT_EQUAL_INT(8, w.size());
    TEST_ASSERT_EQUAL_INT(0, w.progress());
}

/**
 * Test: Construction with zero size (edge case)
 */
void test_wind360_construction_zero_size(void) {
    Wind360 w(0);
    TEST_ASSERT_EQUAL_INT(0, w.size());
    TEST_ASSERT_EQUAL_INT(0, w.progress());
    TEST_ASSERT_FALSE(w.set_degree(45.0));  // Should fail
}

/**
 * Test: buffer_size calculation
 * For N buckets, we need ceil(N/8) bytes to pack all bits
 */
void test_wind360_buffer_size(void) {
    Wind360 w8(8);    // 8 buckets  -> ceil(8/8) = 1 byte
    TEST_ASSERT_EQUAL_INT(1, w8.buffer_size());

    Wind360 w16(16);  // 16 buckets -> ceil(16/8) = 2 bytes
    TEST_ASSERT_EQUAL_INT(2, w16.buffer_size());

    Wind360 w9(9);    // 9 buckets  -> ceil(9/8) = 2 bytes
    TEST_ASSERT_EQUAL_INT(2, w9.buffer_size());

    Wind360 w90(90);  // 90 buckets -> ceil(90/8) = 12 bytes
    TEST_ASSERT_EQUAL_INT(12, w90.buffer_size());
}

/**
 * Test: set_degree and progress tracking
 * Adding same angle twice should return false the second time
 */
void test_wind360_set_degree_new_angles(void) {
    Wind360 w(8);  // 8 buckets, 45° each
    
    // First angle: 0° (bucket 0)
    bool result = w.set_degree(0.0);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(1, w.progress());

    // Different angle: 45° (bucket 1)
    result = w.set_degree(45.0);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(2, w.progress());

    // Different angle: 90° (bucket 2)
    result = w.set_degree(90.0);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(3, w.progress());
}

/**
 * Test: set_degree rejects duplicate angles
 * Same angle should return false on second attempt
 */
void test_wind360_set_degree_duplicate(void) {
    Wind360 w(8);

    bool first = w.set_degree(22.5);  // Between bucket 0 and 1, rounds to 0
    TEST_ASSERT_TRUE(first);
    TEST_ASSERT_EQUAL_INT(1, w.progress());

    bool second = w.set_degree(22.5);  // Same angle again
    TEST_ASSERT_FALSE(second);
    TEST_ASSERT_EQUAL_INT(1, w.progress());  // Progress unchanged
}

/**
 * Test: Angle normalization and bucketing
 * Angles are normalized to [0, 360) and mapped to buckets
 */
void test_wind360_angle_normalization(void) {
    Wind360 w(8);  // 8 buckets, 45° each
    
    // 0° and 360° should map to same bucket
    TEST_ASSERT_TRUE(w.set_degree(0.0));
    TEST_ASSERT_EQUAL_INT(1, w.progress());
    TEST_ASSERT_FALSE(w.set_degree(360.0));  // Same bucket, should fail
    
    // Negative angles should wrap
    Wind360 w2(8);
    TEST_ASSERT_TRUE(w2.set_degree(-45.0));  // Should map to 315°
    TEST_ASSERT_EQUAL_INT(1, w2.progress());
    TEST_ASSERT_FALSE(w2.set_degree(315.0));  // Same bucket
}

/**
 * Test: reset clears all data and counters
 */
void test_wind360_reset(void) {
    Wind360 w(8);
    
    // Add some data
    w.set_degree(0.0);
    w.set_degree(45.0);
    w.set_degree(90.0);
    TEST_ASSERT_EQUAL_INT(3, w.progress());
    
    // Reset should clear everything
    w.reset();
    TEST_ASSERT_EQUAL_INT(0, w.progress());
    TEST_ASSERT_EQUAL_DOUBLE(0.0, w.get_score());
    
    // Should be able to add same angles again
    TEST_ASSERT_TRUE(w.set_degree(0.0));
    TEST_ASSERT_TRUE(w.set_degree(45.0));
}

/**
 * Test: is_valid returns true only when all buckets have samples
 */
void test_wind360_is_valid(void) {
    Wind360 w(4);  // 4 buckets, easy to test
    
    TEST_ASSERT_FALSE(w.is_valid());  // Empty
    TEST_ASSERT_EQUAL_INT(0, w.progress());

    w.set_degree(0.0);    // Bucket 0
    TEST_ASSERT_FALSE(w.is_valid());
    TEST_ASSERT_EQUAL_INT(1, w.progress());

    w.set_degree(90.0);   // Bucket 1
    TEST_ASSERT_FALSE(w.is_valid());
    TEST_ASSERT_EQUAL_INT(2, w.progress());

    w.set_degree(180.0);  // Bucket 2
    TEST_ASSERT_FALSE(w.is_valid());
    TEST_ASSERT_EQUAL_INT(3, w.progress());

    w.set_degree(270.0);  // Bucket 3 (last one)
    TEST_ASSERT_TRUE(w.is_valid());
    TEST_ASSERT_EQUAL_INT(4, w.progress());
}

/**
 * Test: get_data serializes bucket state into bytes
 * Each bit in a byte represents one bucket (8 buckets per byte)
 */
void test_wind360_get_data_serialization(void) {
    Wind360 w(8);  // 8 buckets = 1 byte
    
    // Set bucket 0 (bit 0) and bucket 2 (bit 2)
    w.set_degree(0.0);    // Bucket 0
    w.set_degree(90.0);   // Bucket 2
    
    unsigned char byte0 = w.get_data(0);
    TEST_ASSERT_EQUAL_INT(0x05, byte0);  // Binary: 00000101 (bits 0 and 2 set)
}

/**
 * Test: get_data with multiple bytes
 * For 16 buckets, we need 2 bytes
 */
void test_wind360_get_data_multiple_bytes(void) {
    Wind360 w(16);  // 16 buckets = 2 bytes
    
    // Set buckets in first byte: 0, 1, 7
    w.set_degree(0.0);     // Bucket 0 (bit 0 in byte 0)
    w.set_degree(22.5);    // Bucket 1 (bit 1 in byte 0)
    w.set_degree(157.5);   // Bucket 7 (bit 7 in byte 0)
    
    unsigned char byte0 = w.get_data(0);
    TEST_ASSERT_EQUAL_INT(0x83, byte0);  // Binary: 10000011
    
    // Set buckets in second byte: 8
    w.set_degree(180.0);   // Bucket 8 (bit 0 in byte 1)
    
    unsigned char byte1 = w.get_data(1);
    TEST_ASSERT_EQUAL_INT(0x01, byte1);  // Binary: 00000001
}

/**
 * Test: get_data bounds checking
 * Requesting byte index outside buffer should return 0
 */
void test_wind360_get_data_bounds(void) {
    Wind360 w(8);  // 1 byte buffer
    
    w.set_degree(0.0);
    TEST_ASSERT_NOT_EQUAL(0, w.get_data(0));
    TEST_ASSERT_EQUAL_INT(0, w.get_data(1));   // Out of bounds
    TEST_ASSERT_EQUAL_INT(0, w.get_data(-1));  // Negative index
    TEST_ASSERT_EQUAL_INT(0, w.get_data(100)); // Way out of bounds
}

/**
 * Test: get_data with zero-size Wind360
 */
void test_wind360_get_data_empty(void) {
    Wind360 w(0);
    TEST_ASSERT_EQUAL_INT(0, w.get_data(0));
}

/**
 * Test: Full bucket sweep
 * Fill all 90 default buckets and verify valid state
 */
void test_wind360_full_sweep(void) {
    Wind360 w(90);  // 90 buckets, 4° each
    
    // Add one sample per bucket by spacing 4° apart
    for (int i = 0; i < 90; i++) {
        double angle = i * 4.0;  // 0°, 4°, 8°, ..., 356°
        bool result = w.set_degree(angle);
        TEST_ASSERT_TRUE(result);
    }
    
    TEST_ASSERT_EQUAL_INT(90, w.progress());
    TEST_ASSERT_TRUE(w.is_valid());
    TEST_ASSERT_GREATER_THAN_DOUBLE(0.0, w.get_score());  // Should have non-zero score
}

/**
 * Test: Ellipse parameter (signal quality)
 * Ellipse value should be stored in scores array
 */
void test_wind360_ellipse_score(void) {
    Wind360 w(4);
    
    // Set angles with different ellipse (quality) values
    w.set_degree(0.0, 0.95);    // High quality
    w.set_degree(90.0, 0.50);   // Low quality
    w.set_degree(180.0, 1.0);   // Perfect
    w.set_degree(270.0, 0.0);   // No quality
    
    TEST_ASSERT_TRUE(w.is_valid());
    // The ellipse values are stored but not directly exposed in public API
    // This test verifies they don't cause errors
}

/**
 * Test: get_score calculation
 * Score should be normalized by tot_score
 */
void test_wind360_score_calculation(void) {
    Wind360 w(4);
    
    TEST_ASSERT_EQUAL_DOUBLE(0.0, w.get_score());
    
    // Add samples - score increases
    w.set_degree(0.0);
    double score1 = w.get_score();
    TEST_ASSERT_GREATER_THAN_DOUBLE(0.0, score1);
    
    w.set_degree(90.0);
    double score2 = w.get_score();
    TEST_ASSERT_GREATER_THAN_DOUBLE(score1, score2);  // Should increase
}

void test_wind360_bucket_allocation(void) {
    Wind360 w(4);  // 4 buckets

    TEST_ASSERT_EQUAL_INT(0, w.get_angle_bucket(315.0));  // Bucket 0
    TEST_ASSERT_EQUAL_INT(0, w.get_angle_bucket(0.0));    // Bucket 0
    TEST_ASSERT_EQUAL_INT(0, w.get_angle_bucket(44.9));   // Bucket 0
    
    TEST_ASSERT_EQUAL_INT(1, w.get_angle_bucket(45.0));   // Bucket 1
    TEST_ASSERT_EQUAL_INT(1, w.get_angle_bucket(90.0));   // Bucket 1
    TEST_ASSERT_EQUAL_INT(1, w.get_angle_bucket(134.9));  // Bucket 1
    
    TEST_ASSERT_EQUAL_INT(2, w.get_angle_bucket(135.0));  // Bucket 2
    TEST_ASSERT_EQUAL_INT(2, w.get_angle_bucket(180.0));  // Bucket 2
    TEST_ASSERT_EQUAL_INT(2, w.get_angle_bucket(224.9));  // Bucket 2
    
    TEST_ASSERT_EQUAL_INT(3, w.get_angle_bucket(225.0));  // Bucket 3
    TEST_ASSERT_EQUAL_INT(3, w.get_angle_bucket(270.0));  // Bucket 3
    TEST_ASSERT_EQUAL_INT(3, w.get_angle_bucket(314.9));  // Bucket 3
}   



/**
 * Test: Bucket rounding
 * Angles within a bucket should round to the same bucket
 */
void test_wind360_bucket_rounding(void) {
    Wind360 w(8);  // 45° per bucket
    
    // All angles 337.5-22.5° should map to bucket 0
    TEST_ASSERT_TRUE(w.set_degree(337.6));     // Start of bucket
    TEST_ASSERT_FALSE(w.set_degree(0));   // Middle of bucket
    TEST_ASSERT_FALSE(w.set_degree(22.4));  // Almost next
    TEST_ASSERT_TRUE(w.set_degree(22.5));    // Next bucket
}

/**
 * Test: Large bucket sizes
 */
void test_wind360_large_bucket_size(void) {
    Wind360 w(360);  // 1° per bucket
    
    TEST_ASSERT_EQUAL_INT(360, w.size());
    TEST_ASSERT_EQUAL_INT(45, w.buffer_size());  // ceil(360/8) = 45
    
    // Add samples every 10°
    for (int i = 0; i < 36; i++) {
        TEST_ASSERT_TRUE(w.set_degree(i * 10.0));
    }
    
    TEST_ASSERT_EQUAL_INT(36, w.progress());
    TEST_ASSERT_FALSE(w.is_valid());  // Only 36/360 filled
}

/**
 * Test: Small bucket size
 */
void test_wind360_small_bucket_size(void) {
    Wind360 w(1);  // Single bucket (entire 360°)
    
    TEST_ASSERT_EQUAL_INT(1, w.size());
    TEST_ASSERT_EQUAL_INT(1, w.buffer_size());
    
    // Any angle should map to bucket 0
    TEST_ASSERT_TRUE(w.set_degree(0.0));
    TEST_ASSERT_FALSE(w.set_degree(179.9));   // Different angle, same bucket
    TEST_ASSERT_FALSE(w.set_degree(359.9));   // Another angle, same bucket
    
    TEST_ASSERT_TRUE(w.is_valid());  // Single bucket, now valid
}

// Run all tests
void setup(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_wind360_construction_default);
    RUN_TEST(test_wind360_construction_custom);
    RUN_TEST(test_wind360_construction_zero_size);
    RUN_TEST(test_wind360_buffer_size);
    RUN_TEST(test_wind360_set_degree_new_angles);
    RUN_TEST(test_wind360_set_degree_duplicate);
    RUN_TEST(test_wind360_angle_normalization);
    RUN_TEST(test_wind360_reset);
    RUN_TEST(test_wind360_is_valid);
    RUN_TEST(test_wind360_get_data_serialization);
    RUN_TEST(test_wind360_get_data_multiple_bytes);
    RUN_TEST(test_wind360_get_data_bounds);
    RUN_TEST(test_wind360_get_data_empty);
    RUN_TEST(test_wind360_full_sweep);
    RUN_TEST(test_wind360_ellipse_score);
    RUN_TEST(test_wind360_score_calculation);
    RUN_TEST(test_wind360_bucket_allocation);
    RUN_TEST(test_wind360_bucket_rounding);
    RUN_TEST(test_wind360_large_bucket_size);
    RUN_TEST(test_wind360_small_bucket_size);
    
    UNITY_END();
}

void loop(void) {
    // Empty
}

int main(int argc, char **argv) {
    setup();
    return 0;
}