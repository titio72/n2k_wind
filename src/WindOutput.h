#ifndef _WIND_OUTPUT_H
#define _WIND_OUTPUT_H

#include <stdint.h>
#include "WindCorrection.h"

// One PGN 130306 sample: the values to send and the status byte (byte 7).
struct wind_output
{
    double awa;
    double aws;
    bool corrected;
    uint8_t status; // wind_status_byte()
};

// Corrects the sample when STW is valid and the table is usable; otherwise
// passes it through unchanged with status 0.
wind_output compute_wind_output(const correction_table &t, uint32_t table_id,
                                double awa_deg, double aws_kn, bool stw_valid, double stw_kn);

#endif
