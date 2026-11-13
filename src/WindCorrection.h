#ifndef _WIND_CORRECTION_H
#define _WIND_CORRECTION_H

#include <stdint.h>

// True-wind correction table (NKE-style "upwash" table).
// Rows are indexed by true wind speed; each row has 3 anchors at true wind
// angles. Corrections are applied to true wind computed from STW, and the
// apparent wind is recomputed from the corrected true wind.

#define CORRECTION_MAX_ROWS 10
#define CORRECTION_ANCHORS 3

#define CORRECTION_MAX_TWS 1000     // 100.0 kn
#define CORRECTION_MAX_D_ANGLE 300  // 30.0 deg
#define CORRECTION_MAX_D_SPEED 100  // 10.0 kn

struct correction_anchor
{
    uint8_t angle;   // deg, 1..179
    int16_t d_angle; // 0.1 deg, positive widens |TWA|
    int16_t d_speed; // 0.1 kn
};

struct correction_row
{
    uint16_t tws; // 0.1 kn
    correction_anchor anchors[CORRECTION_ANCHORS];
};

struct correction_table
{
    uint8_t enabled;
    uint8_t n_rows;
    correction_row rows[CORRECTION_MAX_ROWS];
};

void correction_clear(correction_table &t);
bool correction_row_valid(const correction_row &r);
bool correction_table_valid(const correction_table &t);

// false: invalid row or table full (table unchanged)
bool correction_upsert(correction_table &t, const correction_row &r);
// false: no row with that tws
bool correction_delete(correction_table &t, uint16_t tws);
// "tws,a1,dA1,dS1,a2,dA2,dS2,a3,dA3,dS3" in display units (kn, deg)
bool parse_correction_row(const char *s, correction_row &r);

// Corrections (deg, kn) at the given true wind speed and |TWA| (0..180).
void correction_lookup(const correction_table &t, double tws_kn, double abs_twa_deg,
                       double &d_angle_deg, double &d_speed_kn);

// Applies the table to an apparent wind sample. Returns false (and copies the
// inputs to the outputs) when the table is disabled or empty, or any input is
// NaN. out_awa_deg is in [0,360).
bool correct_wind(const correction_table &t, double awa_deg, double aws_kn, double stw_kn,
                  double &out_awa_deg, double &out_aws_kn);

#endif
