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
    N2KWind(n2k_source_change_handler on_src) : n2k(*N2K::get_instance(nullptr, on_src)), src(DEFAULT_N2K_SOURCE), no_stats(true), n2k_err(false)
    {
    }

    void setup()
    {
        if (N2K_ENABLED)
        {
            n2k.set_desired_source(src);
            n2k.add_pgn(130306L);
            n2k_device_info info;
            info.ModelSerialCode = N2K_MODEL_SERIAL_CODE;
            info.ProductCode = N2K_PRODUCT_CODE;
            info.ModelID = N2K_MODEL_ID;
            info.SwCode = N2K_SW_CODE;
            info.ModelVersion = N2K_MODEL_VERSION;
            info.UniqueNumber = N2K_UNIQUE_NUMBER;
            info.DeviceFunction = N2K_DEVICEE_FUNCTION;
            info.DeviceClass = N2K_DEVICE_CLASS;
            info.ManufacturerCode = N2K_MANIFACTURER_CODE;
            n2k.setup(info);
        }
    }

    void set_source(uint8_t s)
    {
        src = s;
        n2k.set_desired_source(src);
    }

    void send_N2K(double awd_deg, double aws_kn)
    {
        if (N2K_ENABLED)
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
                no_stats = false;
                n2k_err = !n2k.is_bus_connected();
            }
            else
            {
                n2k_err = s.fail>last_stats.fail || !n2k.is_bus_connected();
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
