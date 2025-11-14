#ifndef _N2K_WIND_H
#define _N2K_WIND_H

#include <N2K.h>
#include <Log.h>
#include "WindUtil.h"

#ifndef N2K_ENABLED
#define N2K_ENABLED true
#endif

class N2KWind
{
public:
    N2KWind(void (*on_src)(uint8_t old_src, uint8_t new_src)) : n2k(*N2K::get_instance(NULL, on_src)), src(DEFAULT_WIND_N2K_SOURCE), no_stats(true)
    {
    }

    void setup()
    {
        if (N2K_ENABLED)
        {
            n2k.set_desired_source(src);
            n2k.add_pgn(130306L);
            n2k_device_info info;
            info.ModelSerialCode = "0.0.1";
            info.ProductCode = 101;
            info.ModelID = "ABWind";
            info.SwCode = "0.0.1";
            info.ModelVersion = "0001";
            info.UniqueNumber = 2;
            info.DeviceFunction = 180;
            info.DeviceClass = 60;
            info.ManufacturerCode = 2046;
            n2k.setup(info);
        }
    }

    void send_N2K(double awd_deg, double aws_kn, unsigned long time)
    {
        if (N2K_ENABLED) // && wdata.error == WIND_ERROR_OK)
        {
            tN2kMsg msg(n2k.get_source());
            SetN2kWindSpeed(msg, 0, KnotsToms(aws_kn), DegToRad(awd_deg), tN2kWindReference::N2kWind_Apparent);
            n2k.send_msg(msg);
        }
    }

    void loop(unsigned long milliseconds)
    {
        if (N2K_ENABLED)
        {
            n2k.loop(milliseconds);

            N2KStats s = n2k.getStats();
            if (no_stats)
            {
                last_stats = s;
            }
            else
            {
                n2k_err = (s.fail>last_stats.fail);
            }
        }
    }

    bool is_n2k_err()
    {
        return n2k_err;
    }

private:
    N2K &n2k;
    uint8_t src;
    N2KStats last_stats;
    bool no_stats;
    bool n2k_err;
};

#endif
