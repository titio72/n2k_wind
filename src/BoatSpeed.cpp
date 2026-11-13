#include "BoatSpeed.h"
#include <math.h>

bool BoatSpeed::fresh(unsigned long now_ms) const
{
    return has_value && (now_ms - last_ms) <= STW_FRESHNESS_MS;
}

void BoatSpeed::on_stw(double stw_kn, uint8_t source, unsigned long now_ms)
{
    if (isnan(stw_kn))
        return;
    if (fresh(now_ms) && source != src)
        return;
    src = source;
    stw = stw_kn;
    last_ms = now_ms;
    has_value = true;
}

bool BoatSpeed::get(unsigned long now_ms, double &stw_kn) const
{
    if (!fresh(now_ms))
        return false;
    stw_kn = stw;
    return true;
}
