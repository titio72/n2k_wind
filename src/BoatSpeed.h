#ifndef _BOAT_SPEED_H
#define _BOAT_SPEED_H

#include <stdint.h>

#define STW_FRESHNESS_MS 3000UL

// Speed through water from PGN 128259, with a freshness window.
// The first source seen is kept while it is fresh; other sources are ignored
// until it goes stale, then the next message from any source is adopted.
class BoatSpeed
{
public:
    void on_stw(double stw_kn, uint8_t source, unsigned long now_ms); // NaN ignored
    bool get(unsigned long now_ms, double &stw_kn) const;             // false if never received or stale

private:
    bool fresh(unsigned long now_ms) const;

    bool has_value = false;
    uint8_t src = 0;
    double stw = 0.0;
    unsigned long last_ms = 0;
};

#endif
