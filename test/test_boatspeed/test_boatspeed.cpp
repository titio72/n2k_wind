#include <unity.h>
#include <math.h>
#include <limits.h>
#include "../src/BoatSpeed.h"

void setUp(void) {}
void tearDown(void) {}

void test_no_message(void)
{
    BoatSpeed b;
    double stw = 99;
    TEST_ASSERT_FALSE(b.get(0, stw));
    TEST_ASSERT_FALSE(b.get(10000, stw));
}

void test_freshness(void)
{
    BoatSpeed b;
    double stw = 0;
    b.on_stw(5.2, 35, 1000);
    TEST_ASSERT_TRUE(b.get(3999, stw));
    TEST_ASSERT_EQUAL_DOUBLE(5.2, stw);
    TEST_ASSERT_FALSE(b.get(4001, stw));
}

void test_source_stickiness(void)
{
    BoatSpeed b;
    double stw = 0;
    b.on_stw(5.2, 35, 1000);
    b.on_stw(9.9, 40, 2000); // ignored, 35 is fresh
    TEST_ASSERT_TRUE(b.get(2500, stw));
    TEST_ASSERT_EQUAL_DOUBLE(5.2, stw);
    TEST_ASSERT_FALSE(b.get(4500, stw)); // 40's message did not refresh

    b.on_stw(4.0, 40, 5000); // 35 stale: adopt 40
    TEST_ASSERT_TRUE(b.get(5000, stw));
    TEST_ASSERT_EQUAL_DOUBLE(4.0, stw);
    b.on_stw(6.0, 35, 5500); // now 35 is the stranger
    TEST_ASSERT_TRUE(b.get(5600, stw));
    TEST_ASSERT_EQUAL_DOUBLE(4.0, stw);
}

void test_nan_does_not_refresh(void)
{
    BoatSpeed b;
    double stw = 0;
    b.on_stw(5.2, 35, 1000);
    b.on_stw(NAN, 35, 3500);
    TEST_ASSERT_FALSE(b.get(4001, stw));
}

void test_clock_wrap(void)
{
    BoatSpeed b;
    double stw = 0;
    b.on_stw(3.0, 35, ULONG_MAX - 500);
    TEST_ASSERT_TRUE(b.get(1000, stw));
    TEST_ASSERT_EQUAL_DOUBLE(3.0, stw);
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_no_message);
    RUN_TEST(test_freshness);
    RUN_TEST(test_source_stickiness);
    RUN_TEST(test_nan_does_not_refresh);
    RUN_TEST(test_clock_wrap);
    return UNITY_END();
}
