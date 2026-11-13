#include <unity.h>
#include <string.h>
#include "../src/WindCorrection.h"
#include "../src/CorrectionProtocol.h"

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

static const uint8_t T1_PAYLOAD[] = {
    0xFE, 0x9F, 0x01, 0x01, 0xDE, 0xA1, 0x30, 0x53, 0x01, 0x02, 0x3C, 0x00, 0x2D, 0xC4, 0xFF,
    0x00, 0x00, 0x5F, 0xD8, 0xFF, 0x00, 0x00, 0x96, 0xEC, 0xFF, 0x00, 0x00, 0x64, 0x00, 0x28,
    0xDD, 0xFF, 0x00, 0x00, 0x5F, 0xF6, 0xFF, 0x00, 0x00, 0x96, 0x14, 0x00, 0x05, 0x00};

void setUp(void) {}
void tearDown(void) {}

void test_crc32_check_value(void)
{
    TEST_ASSERT_EQUAL_HEX32(0xCBF43926, crc32_ieee((const uint8_t *)"123456789", 9));
}

void test_encode_t1_golden(void)
{
    correction_table t = make_t1();
    uint8_t buf[CORRECTION_MAX_PAYLOAD];
    size_t n = encode_correction_table(t, 2046, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_UINT32(sizeof(T1_PAYLOAD), n);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(T1_PAYLOAD, buf, sizeof(T1_PAYLOAD));
}

void test_table_ids(void)
{
    correction_table t = make_t1();
    TEST_ASSERT_EQUAL_HEX32(0x5330A1DE, correction_table_id(t));
    t.enabled = 0;
    TEST_ASSERT_EQUAL_HEX32(0xC640754B, correction_table_id(t));
    correction_clear(t);
    TEST_ASSERT_EQUAL_HEX32(0x41D912FF, correction_table_id(t));
}

void test_full_table_size(void)
{
    correction_table t;
    correction_clear(t);
    for (int i = 0; i < CORRECTION_MAX_ROWS; i++)
        correction_upsert(t, {(uint16_t)(10 * (i + 1)), {{40, -35, 0}, {95, -10, 0}, {150, 20, 5}}});
    TEST_ASSERT_EQUAL_UINT8(CORRECTION_MAX_ROWS, t.n_rows);
    uint8_t buf[CORRECTION_MAX_PAYLOAD];
    TEST_ASSERT_EQUAL_UINT32(180, encode_correction_table(t, 2046, buf, sizeof(buf)));
}

void test_buffer_too_small(void)
{
    correction_table t = make_t1();
    uint8_t buf[CORRECTION_MAX_PAYLOAD];
    TEST_ASSERT_EQUAL_UINT32(0, encode_correction_table(t, 2046, buf, 43));
}

void test_reference_byte(void)
{
    TEST_ASSERT_EQUAL_HEX8(0xEA, wind_reference_byte(true, 0x5330A1DE));
    TEST_ASSERT_EQUAL_HEX8(0x02, wind_reference_byte(false, 0x5330A1DE));
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_crc32_check_value);
    RUN_TEST(test_encode_t1_golden);
    RUN_TEST(test_table_ids);
    RUN_TEST(test_full_table_size);
    RUN_TEST(test_buffer_too_small);
    RUN_TEST(test_reference_byte);
    return UNITY_END();
}
