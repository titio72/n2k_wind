#include <unity.h>
#include <math.h>
#include "../src/WindCorrection.h"
#include "../src/WindOutput.h"

#define T1_ID 0x5330A1DEUL

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

void setUp(void) {}
void tearDown(void) {}

static void assert_uncorrected(const wind_output &o, double awa, double aws)
{
    TEST_ASSERT_FALSE(o.corrected);
    TEST_ASSERT_EQUAL_HEX8(0x02, o.reference);
    TEST_ASSERT_EQUAL_DOUBLE(awa, o.awa);
    TEST_ASSERT_EQUAL_DOUBLE(aws, o.aws);
}

void test_corrected_sample(void)
{
    wind_output o = compute_wind_output(make_t1(), T1_ID, 40, 15, true, 6);
    TEST_ASSERT_TRUE(o.corrected);
    TEST_ASSERT_EQUAL_HEX8(0xEA, o.reference);
    TEST_ASSERT_DOUBLE_WITHIN(1e-3, 38.2193, o.awa);
    TEST_ASSERT_DOUBLE_WITHIN(1e-3, 15.1701, o.aws);
}

void test_no_stw(void)
{
    wind_output o = compute_wind_output(make_t1(), T1_ID, 40, 15, false, 6);
    assert_uncorrected(o, 40, 15);
}

void test_nan_wind(void)
{
    wind_output o = compute_wind_output(make_t1(), T1_ID, NAN, 15, true, 6);
    TEST_ASSERT_FALSE(o.corrected);
    TEST_ASSERT_EQUAL_HEX8(0x02, o.reference);
    TEST_ASSERT_TRUE(isnan(o.awa));
    TEST_ASSERT_EQUAL_DOUBLE(15, o.aws);
}

void test_table_disabled(void)
{
    correction_table t = make_t1();
    t.enabled = 0;
    assert_uncorrected(compute_wind_output(t, T1_ID, 40, 15, true, 6), 40, 15);
}

void test_table_empty(void)
{
    correction_table t;
    correction_clear(t);
    t.enabled = 1;
    assert_uncorrected(compute_wind_output(t, T1_ID, 40, 15, true, 6), 40, 15);
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_corrected_sample);
    RUN_TEST(test_no_stw);
    RUN_TEST(test_nan_wind);
    RUN_TEST(test_table_disabled);
    RUN_TEST(test_table_empty);
    return UNITY_END();
}
