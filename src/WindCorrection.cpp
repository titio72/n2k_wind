#include "WindCorrection.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define DEG_TO_RAD_F (M_PI / 180.0)
#define RAD_TO_DEG_F (180.0 / M_PI)

void correction_clear(correction_table &t)
{
    memset(&t, 0, sizeof(t));
}

// |delta_v| <= 0.5 * delta_x, with v and x in the same 0.1 units
static bool slope_ok(long delta_v, long delta_x_tenths)
{
    return 2 * labs(delta_v) <= delta_x_tenths;
}

bool correction_row_valid(const correction_row &r)
{
    if (r.tws > CORRECTION_MAX_TWS)
        return false;
    int prev_angle = 0;
    int prev_da = 0; // taper from 0 at 0 deg
    for (int i = 0; i < CORRECTION_ANCHORS; i++)
    {
        const correction_anchor &a = r.anchors[i];
        if (a.angle <= prev_angle || a.angle >= 180)
            return false;
        if (abs(a.d_angle) > CORRECTION_MAX_D_ANGLE || abs(a.d_speed) > CORRECTION_MAX_D_SPEED)
            return false;
        if (!slope_ok(a.d_angle - prev_da, 10L * (a.angle - prev_angle)))
            return false;
        if (!slope_ok(a.d_speed, r.tws))
            return false;
        prev_angle = a.angle;
        prev_da = a.d_angle;
    }
    // taper to 0 at 180 deg
    return slope_ok(prev_da, 10L * (180 - prev_angle));
}

bool correction_table_valid(const correction_table &t)
{
    if (t.enabled > 1 || t.n_rows > CORRECTION_MAX_ROWS)
        return false;
    for (int i = 0; i < t.n_rows; i++)
    {
        if (!correction_row_valid(t.rows[i]))
            return false;
        if (i > 0)
        {
            const correction_row &a = t.rows[i - 1], &b = t.rows[i];
            if (b.tws <= a.tws)
                return false;
            for (int j = 0; j < CORRECTION_ANCHORS; j++)
                if (!slope_ok(b.anchors[j].d_speed - a.anchors[j].d_speed, b.tws - a.tws))
                    return false;
        }
    }
    return true;
}

bool correction_upsert(correction_table &t, const correction_row &r)
{
    if (!correction_row_valid(r))
        return false;
    int pos = 0;
    while (pos < t.n_rows && t.rows[pos].tws < r.tws)
        pos++;
    if (pos < t.n_rows && t.rows[pos].tws == r.tws)
    {
        correction_table edited = t;
        edited.rows[pos] = r;
        if (!correction_table_valid(edited))
            return false;
        t = edited;
        return true;
    }
    if (t.n_rows >= CORRECTION_MAX_ROWS)
        return false;
    correction_table edited = t;
    for (int i = edited.n_rows; i > pos; i--)
        edited.rows[i] = edited.rows[i - 1];
    edited.rows[pos] = r;
    edited.n_rows++;
    if (!correction_table_valid(edited))
        return false;
    t = edited;
    return true;
}

bool correction_delete(correction_table &t, uint16_t tws)
{
    for (int i = 0; i < t.n_rows; i++)
    {
        if (t.rows[i].tws == tws)
        {
            for (int j = i; j < t.n_rows - 1; j++)
                t.rows[j] = t.rows[j + 1];
            t.n_rows--;
            memset(&t.rows[t.n_rows], 0, sizeof(correction_row));
            return true;
        }
    }
    return false;
}

static bool parse_number(const char *&p, char terminator, double &v)
{
    if (*p == '\0' || *p == ',')
        return false;
    char *end = nullptr;
    v = strtod(p, &end);
    if (end == p || *end != terminator || !isfinite(v))
        return false;
    p = (*end == '\0') ? end : end + 1;
    return true;
}

static bool to_int(double v, double scale, long lo, long hi, long &out)
{
    out = lround(v * scale);
    return out >= lo && out <= hi;
}

bool parse_correction_row(const char *s, correction_row &r)
{
    if (s == nullptr)
        return false;
    const int n_fields = 1 + 3 * CORRECTION_ANCHORS;
    double v[n_fields];
    const char *p = s;
    for (int i = 0; i < n_fields; i++)
    {
        if (!parse_number(p, (i == n_fields - 1) ? '\0' : ',', v[i]))
            return false;
    }

    correction_row tmp;
    long x;
    if (!to_int(v[0], 10.0, 0, CORRECTION_MAX_TWS, x))
        return false;
    tmp.tws = (uint16_t)x;
    for (int i = 0; i < CORRECTION_ANCHORS; i++)
    {
        if (!to_int(v[1 + 3 * i], 1.0, 0, 255, x))
            return false;
        tmp.anchors[i].angle = (uint8_t)x;
        if (!to_int(v[2 + 3 * i], 10.0, -CORRECTION_MAX_D_ANGLE, CORRECTION_MAX_D_ANGLE, x))
            return false;
        tmp.anchors[i].d_angle = (int16_t)x;
        if (!to_int(v[3 + 3 * i], 10.0, -CORRECTION_MAX_D_SPEED, CORRECTION_MAX_D_SPEED, x))
            return false;
        tmp.anchors[i].d_speed = (int16_t)x;
    }
    if (!correction_row_valid(tmp))
        return false;
    r = tmp;
    return true;
}

// Correction within one row at angle x (0..180), tapering to 0 at 0 and 180.
static void row_lookup(const correction_row &r, double x, double &da, double &ds)
{
    const correction_anchor *a = r.anchors;
    const int last = CORRECTION_ANCHORS - 1;
    if (x <= a[0].angle)
    {
        double f = x / a[0].angle;
        da = a[0].d_angle * f;
        ds = a[0].d_speed * f;
    }
    else if (x >= a[last].angle)
    {
        double f = (180.0 - x) / (180.0 - a[last].angle);
        da = a[last].d_angle * f;
        ds = a[last].d_speed * f;
    }
    else
    {
        int i = 0;
        while (x > a[i + 1].angle)
            i++;
        double f = (x - a[i].angle) / (double)(a[i + 1].angle - a[i].angle);
        da = a[i].d_angle + (a[i + 1].d_angle - a[i].d_angle) * f;
        ds = a[i].d_speed + (a[i + 1].d_speed - a[i].d_speed) * f;
    }
    da /= 10.0;
    ds /= 10.0;
}

void correction_lookup(const correction_table &t, double tws_kn, double abs_twa_deg,
                       double &d_angle_deg, double &d_speed_kn)
{
    d_angle_deg = 0.0;
    d_speed_kn = 0.0;
    if (t.n_rows == 0 || isnan(tws_kn) || isnan(abs_twa_deg))
        return;
    double x = fmin(180.0, fmax(0.0, abs_twa_deg));
    double tws = tws_kn * 10.0;
    int n = t.n_rows;
    if (tws <= t.rows[0].tws)
    {
        row_lookup(t.rows[0], x, d_angle_deg, d_speed_kn);
        return;
    }
    if (tws >= t.rows[n - 1].tws)
    {
        row_lookup(t.rows[n - 1], x, d_angle_deg, d_speed_kn);
        return;
    }
    int i = 0;
    while (tws > t.rows[i + 1].tws)
        i++;
    double da0, ds0, da1, ds1;
    row_lookup(t.rows[i], x, da0, ds0);
    row_lookup(t.rows[i + 1], x, da1, ds1);
    double f = (tws - t.rows[i].tws) / (double)(t.rows[i + 1].tws - t.rows[i].tws);
    d_angle_deg = da0 + (da1 - da0) * f;
    d_speed_kn = ds0 + (ds1 - ds0) * f;
}

bool correct_wind(const correction_table &t, double awa_deg, double aws_kn, double stw_kn,
                  double &out_awa_deg, double &out_aws_kn)
{
    out_awa_deg = awa_deg;
    out_aws_kn = aws_kn;
    if (!t.enabled || t.n_rows == 0 || isnan(awa_deg) || isnan(aws_kn) || isnan(stw_kn))
        return false;
    if (aws_kn < CORRECTION_MIN_AWS_KN)
        return false;

    double a = awa_deg * DEG_TO_RAD_F;
    double tw_x = aws_kn * cos(a) - stw_kn;
    double tw_y = aws_kn * sin(a);
    double tws = hypot(tw_x, tw_y);
    double twa = atan2(tw_y, tw_x) * RAD_TO_DEG_F;
    double sign = (twa < 0) ? -1.0 : 1.0;

    double da, ds;
    correction_lookup(t, tws, fabs(twa), da, ds);
    double twa_c = sign * (fabs(twa) + da) * DEG_TO_RAD_F;
    double tws_c = fmax(0.0, tws + ds);

    double aw_x = tws_c * cos(twa_c) + stw_kn;
    double aw_y = tws_c * sin(twa_c);
    out_aws_kn = hypot(aw_x, aw_y);
    double awa_c = atan2(aw_y, aw_x) * RAD_TO_DEG_F;
    if (awa_c < 0)
        awa_c += 360.0;
    if (awa_c >= 360.0)
        awa_c -= 360.0;
    out_awa_deg = awa_c;
    return true;
}
