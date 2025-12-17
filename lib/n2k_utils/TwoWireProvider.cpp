#ifndef NATIVE

#include "TwoWireProvider.h"
#include "Log.h"

#ifndef SDA_PIN
#define SDA_PIN 16
#endif
#ifndef SCL_PIN
#define SCL_PIN 17
#endif


TwoWire* TwoWireProvider::get_two_wire()
{
    static TwoWire* ic2 = NULL;
    static bool active = false;
    if (!active)
    {
        Log::tracex("I2C", "Initializing TwoWires", "SDA {%d} SCL {%d}", SDA_PIN, SCL_PIN);
        ic2 = &Wire;
        active = ic2->begin(SDA_PIN, SCL_PIN);
        Log::tracex("I2C", "TwoWires Initialized", "active {%d}", active);

        int error, address;
        int nDevices;

        Log::tracex("I2C", "Scanning...");

        nDevices = 0;
        for(address = 1; address < 127; address++ )
        {
            // The i2c_scanner uses the return value of
            // the Write.endTransmisstion to see if
            // a device did acknowledge to the address.
            Wire.beginTransmission(address);
            error = Wire.endTransmission();

            if (error == 0)
            {
                Log::tracex("I2C", "device found", "address 0x%s%x", (address<16)?"0":"", address);
                nDevices++;
            }
            else if (error==4)
            {
                Log::tracex("I2C", "Unknown error at address 0x%s%x", (address<16)?"0":"", address);
            }
        }
        Log::tracex("I2C", "End scan");
    }
    return ic2;

}

#endif
