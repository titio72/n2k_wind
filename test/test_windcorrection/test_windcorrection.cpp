#include <unity.h>
#include <math.h>
#include <string.h>
#include "../src/WindCorrection.h"

#define TOL 1e-3

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

static correction_row make_row(uint16_t tws)
{
    return {tws, {{40, -35, 0}, {95, -10, 0}, {150, 20, 5}}};
}

static double circ_diff(double a, double b)
{
    double d = fmod(a - b + 540.0, 360.0) - 180.0;
    return fabs(d);
}

void setUp(void) {}
void tearDown(void) {}

// ---------------------------------------------------------------- lookup

static void check_lookup(double tws, double twa, double exp_da, double exp_ds)
{
    correction_table t = make_t1();
    double da = NAN, ds = NAN;
    correction_lookup(t, tws, twa, da, ds);
    TEST_ASSERT_DOUBLE_WITHIN(TOL, exp_da, da);
    TEST_ASSERT_DOUBLE_WITHIN(TOL, exp_ds, ds);
}

void test_lookup_on_anchor(void) { check_lookup(10, 40, -3.5, 0.0); }
void test_lookup_taper_bow(void) { check_lookup(10, 20, -1.75, 0.0); }
void test_lookup_zero_at_bow(void) { check_lookup(10, 0, 0.0, 0.0); }
void test_lookup_zero_at_stern(void) { check_lookup(10, 180, 0.0, 0.0); }
void test_lookup_taper_stern(void) { check_lookup(10, 165, 1.0, 0.25); }
void test_lookup_between_anchors(void) { check_lookup(10, 67.5, -2.25, 0.0); }
void test_lookup_between_rows(void) { check_lookup(8, 95, -2.5, 0.0); }
void test_lookup_clamped_below(void) { check_lookup(3, 95, -4.0, 0.0); }
void test_lookup_clamped_above(void) { check_lookup(20, 95, -1.0, 0.0); }

// ---------------------------------------------------------- correct_wind

static void check_correct(double awa, double aws, double stw, double exp_awa, double exp_aws)
{
    correction_table t = make_t1();
    double oa = NAN, os = NAN;
    TEST_ASSERT_TRUE(correct_wind(t, awa, aws, stw, oa, os));
    TEST_ASSERT_DOUBLE_WITHIN(TOL, exp_awa, oa);
    TEST_ASSERT_DOUBLE_WITHIN(TOL, exp_aws, os);
}

void test_correct_upwind(void) { check_correct(40, 15, 6, 38.2193, 15.1701); }
void test_correct_upwind_port_mirror(void) { check_correct(320, 15, 6, 321.7807, 15.1701); }
void test_correct_reaching(void) { check_correct(95, 12, 6.5, 96.0454, 12.1659); }
void test_correct_downwind(void) { check_correct(150, 20, 7.5, 152.0800, 20.2697); }
void test_correct_light_air(void) { check_correct(40, 4, 2, 36.8700, 4.1106); }
void test_correct_stw_zero(void) { check_correct(40, 10, 0, 36.5, 10.0); }
void test_correct_near_stern(void) { check_correct(170, 8, 5, 170.7089, 8.0964); }
void test_correct_strong_wind(void) { check_correct(40, 30, 8, 37.6622, 30.2580); }

void test_correct_continuous_across_bow(void)
{
    correction_table t = make_t1();
    double a1, s1, a2, s2;
    TEST_ASSERT_TRUE(correct_wind(t, 0.1, 15, 6, a1, s1));
    TEST_ASSERT_TRUE(correct_wind(t, 359.9, 15, 6, a2, s2));
    TEST_ASSERT_TRUE(circ_diff(a1, a2) < 0.5);
    TEST_ASSERT_DOUBLE_WITHIN(0.05, s1, s2);
}

void test_correct_continuous_across_stern(void)
{
    correction_table t = make_t1();
    double a1, s1, a2, s2;
    TEST_ASSERT_TRUE(correct_wind(t, 179.9, 15, 6, a1, s1));
    TEST_ASSERT_TRUE(correct_wind(t, 180.1, 15, 6, a2, s2));
    TEST_ASSERT_TRUE(circ_diff(a1, a2) < 0.5);
    TEST_ASSERT_DOUBLE_WITHIN(0.05, s1, s2);
}

static void check_passthrough(const correction_table &t, double awa, double aws, double stw)
{
    double oa = 12345, os = 12345;
    TEST_ASSERT_FALSE(correct_wind(t, awa, aws, stw, oa, os));
    if (isnan(awa)) TEST_ASSERT_TRUE(isnan(oa)); else TEST_ASSERT_EQUAL_DOUBLE(awa, oa);
    if (isnan(aws)) TEST_ASSERT_TRUE(isnan(os)); else TEST_ASSERT_EQUAL_DOUBLE(aws, os);
}

void test_correct_disabled_passthrough(void)
{
    correction_table t = make_t1();
    t.enabled = 0;
    check_passthrough(t, 40, 15, 6);
}

void test_correct_empty_passthrough(void)
{
    correction_table t;
    correction_clear(t);
    t.enabled = 1;
    check_passthrough(t, 40, 15, 6);
}

void test_correct_nan_passthrough(void)
{
    correction_table t = make_t1();
    check_passthrough(t, NAN, 15, 6);
    check_passthrough(t, 40, NAN, 6);
    check_passthrough(t, 40, 15, NAN);
}

// Spec "Testing": correct, then invert (router's fixed-point algorithm),
// gives back the original within 0.05 deg.
static void invert(const correction_table &t, double awa, double aws, double stw,
                   double &raw_awa, double &raw_aws)
{
    double a = awa * M_PI / 180.0;
    double x = aws * cos(a) - stw, y = aws * sin(a);
    double ctws = hypot(x, y);
    double ctwa = atan2(y, x) * 180.0 / M_PI;
    double sign = ctwa < 0 ? -1.0 : 1.0;
    double cabs = fabs(ctwa);
    double rabs = cabs, rtws = ctws;
    for (int i = 0; i < 10; i++)
    {
        double da, ds;
        correction_lookup(t, rtws, rabs, da, ds);
        double nabs = cabs - da, ntws = ctws - ds;
        bool done = fabs(nabs - rabs) < 0.01 && fabs(ntws - rtws) < 0.001;
        rabs = nabs;
        rtws = ntws;
        if (done) break;
    }
    double ra = sign * rabs * M_PI / 180.0;
    double ax = rtws * cos(ra) + stw, ay = rtws * sin(ra);
    raw_aws = hypot(ax, ay);
    raw_awa = fmod(atan2(ay, ax) * 180.0 / M_PI + 360.0, 360.0);
}

void test_roundtrip_inversion(void)
{
    correction_table t = make_t1();
    const double cases[][3] = {{40, 15, 6}, {320, 15, 6}, {95, 12, 6.5}, {150, 20, 7.5},
                               {40, 4, 2}, {170, 8, 5}, {40, 30, 8}, {250, 9, 4}};
    for (auto &c : cases)
    {
        double oa, os, ra, rs;
        TEST_ASSERT_TRUE(correct_wind(t, c[0], c[1], c[2], oa, os));
        invert(t, oa, os, c[2], ra, rs);
        TEST_ASSERT_TRUE(circ_diff(ra, c[0]) < 0.05);
        TEST_ASSERT_DOUBLE_WITHIN(0.01, c[1], rs);
    }
}

// --------------------------------------------------------- edit operations

void test_upsert_orders_and_replaces(void)
{
    correction_table t;
    correction_clear(t);
    TEST_ASSERT_TRUE(correction_upsert(t, make_row(100)));
    TEST_ASSERT_TRUE(correction_upsert(t, make_row(60)));
    TEST_ASSERT_EQUAL_UINT8(2, t.n_rows);
    TEST_ASSERT_EQUAL_UINT16(60, t.rows[0].tws);
    TEST_ASSERT_EQUAL_UINT16(100, t.rows[1].tws);

    correction_row r = make_row(100);
    r.anchors[0].d_angle = -50;
    TEST_ASSERT_TRUE(correction_upsert(t, r));
    TEST_ASSERT_EQUAL_UINT8(2, t.n_rows);
    TEST_ASSERT_EQUAL_INT16(-50, t.rows[1].anchors[0].d_angle);
}

void test_upsert_full_table_rejected(void)
{
    correction_table t;
    correction_clear(t);
    for (int i = 0; i < CORRECTION_MAX_ROWS; i++)
        TEST_ASSERT_TRUE(correction_upsert(t, make_row(10 * (i + 1))));
    correction_table before = t;
    TEST_ASSERT_FALSE(correction_upsert(t, make_row(500)));
    TEST_ASSERT_EQUAL_MEMORY(&before, &t, sizeof(t));
}

void test_upsert_invalid_rows_rejected(void)
{
    correction_table t = make_t1();
    correction_table before = t;
    correction_row r;

    r = make_row(80); r.anchors[0].angle = 0;
    TEST_ASSERT_FALSE(correction_upsert(t, r));
    r = make_row(80); r.anchors[1].angle = r.anchors[0].angle;
    TEST_ASSERT_FALSE(correction_upsert(t, r));
    r = make_row(80); r.anchors[0].d_angle = 301;
    TEST_ASSERT_FALSE(correction_upsert(t, r));
    TEST_ASSERT_EQUAL_MEMORY(&before, &t, sizeof(t));
}

void test_delete(void)
{
    correction_table t = make_t1();
    TEST_ASSERT_TRUE(correction_delete(t, 60));
    TEST_ASSERT_EQUAL_UINT8(1, t.n_rows);
    TEST_ASSERT_EQUAL_UINT16(100, t.rows[0].tws);
    TEST_ASSERT_FALSE(correction_delete(t, 60));
    TEST_ASSERT_EQUAL_UINT8(1, t.n_rows);
}

void test_table_valid(void)
{
    correction_table t = make_t1();
    TEST_ASSERT_TRUE(correction_table_valid(t));
    t.enabled = 2;
    TEST_ASSERT_FALSE(correction_table_valid(t));
    t = make_t1();
    t.rows[1].tws = 60;
    TEST_ASSERT_FALSE(correction_table_valid(t));
    t = make_t1();
    t.n_rows = 11;
    TEST_ASSERT_FALSE(correction_table_valid(t));
}

// ------------------------------------------------------------------ parse

void test_parse_row(void)
{
    correction_row r;
    TEST_ASSERT_TRUE(parse_correction_row("10.0,40,-3.5,0.0,95,-1.0,0.0,150,2.0,0.5", r));
    TEST_ASSERT_EQUAL_UINT16(100, r.tws);
    TEST_ASSERT_EQUAL_UINT8(40, r.anchors[0].angle);
    TEST_ASSERT_EQUAL_INT16(-35, r.anchors[0].d_angle);
    TEST_ASSERT_EQUAL_INT16(0, r.anchors[0].d_speed);
    TEST_ASSERT_EQUAL_UINT8(95, r.anchors[1].angle);
    TEST_ASSERT_EQUAL_INT16(-10, r.anchors[1].d_angle);
    TEST_ASSERT_EQUAL_INT16(0, r.anchors[1].d_speed);
    TEST_ASSERT_EQUAL_UINT8(150, r.anchors[2].angle);
    TEST_ASSERT_EQUAL_INT16(20, r.anchors[2].d_angle);
    TEST_ASSERT_EQUAL_INT16(5, r.anchors[2].d_speed);
}

void test_parse_row_rejects_malformed(void)
{
    correction_row r;
    TEST_ASSERT_FALSE(parse_correction_row("10.0,40,-3.5", r));
    TEST_ASSERT_FALSE(parse_correction_row("10.0,40,-3.5,0.0,95,-1.0,0.0,150,2.0,0.5,1", r));
    TEST_ASSERT_FALSE(parse_correction_row("10.0,40,x,0,95,-1,0,150,2,0.5", r));
    TEST_ASSERT_FALSE(parse_correction_row("", r));
    TEST_ASSERT_FALSE(parse_correction_row("10.0,0,-3.5,0.0,95,-1.0,0.0,150,2.0,0.5", r));
    TEST_ASSERT_FALSE(parse_correction_row("10.0,40,-3.5,0.0,95,-1.0,0.0,180,2.0,0.5", r));
}

int runUnityTests(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_lookup_on_anchor);
    RUN_TEST(test_lookup_taper_bow);
    RUN_TEST(test_lookup_zero_at_bow);
    RUN_TEST(test_lookup_zero_at_stern);
    RUN_TEST(test_lookup_taper_stern);
    RUN_TEST(test_lookup_between_anchors);
    RUN_TEST(test_lookup_between_rows);
    RUN_TEST(test_lookup_clamped_below);
    RUN_TEST(test_lookup_clamped_above);
    RUN_TEST(test_correct_upwind);
    RUN_TEST(test_correct_upwind_port_mirror);
    RUN_TEST(test_correct_reaching);
    RUN_TEST(test_correct_downwind);
    RUN_TEST(test_correct_light_air);
    RUN_TEST(test_correct_stw_zero);
    RUN_TEST(test_correct_near_stern);
    RUN_TEST(test_correct_strong_wind);
    RUN_TEST(test_correct_continuous_across_bow);
    RUN_TEST(test_correct_continuous_across_stern);
    RUN_TEST(test_correct_disabled_passthrough);
    RUN_TEST(test_correct_empty_passthrough);
    RUN_TEST(test_correct_nan_passthrough);
    RUN_TEST(test_roundtrip_inversion);
    RUN_TEST(test_upsert_orders_and_replaces);
    RUN_TEST(test_upsert_full_table_rejected);
    RUN_TEST(test_upsert_invalid_rows_rejected);
    RUN_TEST(test_delete);
    RUN_TEST(test_table_valid);
    RUN_TEST(test_parse_row);
    RUN_TEST(test_parse_row_rejects_malformed);
    return UNITY_END();
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    return runUnityTests();
}
