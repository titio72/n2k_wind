#ifndef _N2K_WIND_H
#define _N2K_WIND_H

#include <N2K.h>
#include <Log.h>
#include "WindUtil.h"
#include "WindCorrection.h"
#include "CorrectionProtocol.h"

#ifndef N2K_ENABLED
#define N2K_ENABLED true
#endif

class N2KWind
{
public:
    N2KWind(n2k_msg_handler on_msg, n2k_source_change_handler on_src) : n2k(*N2K::get_instance(on_msg, on_src)), src(DEFAULT_N2K_SOURCE), no_stats(true), n2k_err(false)
    {
    }

    void setup()
    {
        if (N2K_ENABLED)
        {
            n2k.set_desired_source(src);
            n2k.add_pgn(130306L);
            n2k.add_pgn(CORRECTION_PGN);
            n2k.add_rx_pgn(128259L); // STW for the correction table
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

    // reference: PGN 130306 byte 5 (see wind_reference_byte)
    void send_N2K(double awd_deg, double aws_kn, double ellipse, uint8_t reference)
    {
        if (N2K_ENABLED && isnan(awd_deg) == false && isnan(aws_kn) == false)
        {
            tN2kMsg msg(n2k.get_source());
            //SetN2kWindSpeed(msg, 0, KnotsToms(aws_kn), DegToRad(awd_deg), tN2kWindReference::N2kWind_Apparent);

            msg.SetPGN(130306L);
            msg.Priority=2;
            msg.AddByte(0);
            msg.Add2ByteUDouble(KnotsToms(aws_kn),0.01);
            msg.Add2ByteUDouble(DegToRad(awd_deg),0.0001);
            msg.AddByte(reference);
            msg.Add2ByteUDouble(ellipse,0.01); // Custom data - this last two bytes are "reserved"
            
            n2k.send_msg(msg);
        }
    }

    // Proprietary fast-packet PGN 130900 with the active correction table.
    void send_correction_table(const correction_table &t)
    {
        if (N2K_ENABLED)
        {
            uint8_t buf[CORRECTION_MAX_PAYLOAD];
            size_t len = encode_correction_table(t, N2K_MANIFACTURER_CODE, buf, sizeof(buf));
            if (len == 0)
                return;
            tN2kMsg msg(n2k.get_source());
            msg.SetPGN(CORRECTION_PGN);
            msg.Priority = 7;
            for (size_t i = 0; i < len; i++)
                msg.AddByte(buf[i]);
            bool ok = n2k.send_msg(msg);
            Log::trace("[N2K] Correction table sent {%d bytes} {%s}\n", (int)len, ok ? "OK" : "KO");
        }
    }

    void loop(unsigned long milliseconds)
    {
        if (N2K_ENABLED)
        {
            n2k.loop(milliseconds);

            N2KStats s = n2k.getStats();

            last_stats = s;
            no_stats = false;
            n2k_err = s.fail>0; //!n2k.is_bus_connected();
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
