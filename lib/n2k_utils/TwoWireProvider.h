#ifndef _TWOWIRE_PROVIDER_H
#define _TWOWIRE_PROVIDER_H

#include <Wire.h>

class TwoWire;

class TwoWireProvider
{
public:
    static TwoWire* get_two_wire();

private:
    TwoWireProvider();
};

#endif
