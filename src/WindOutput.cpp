#include "WindOutput.h"
#include "CorrectionProtocol.h"

wind_output compute_wind_output(const correction_table &t, uint32_t table_id,
                                double awa_deg, double aws_kn, bool stw_valid, double stw_kn)
{
    wind_output o = {awa_deg, aws_kn, false, 0};
    if (stw_valid)
        o.corrected = correct_wind(t, awa_deg, aws_kn, stw_kn, o.awa, o.aws);
    o.status = wind_status_byte(o.corrected, table_id);
    return o;
}
