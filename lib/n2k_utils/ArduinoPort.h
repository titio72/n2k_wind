#ifndef LINUXPORT_H_
#define LINUXPORT_H_

#include "Ports.h"
#include <Arduino.h>

class ArduinoPort: public Port
{

public:
    ArduinoPort(HardwareSerial& serial, int rx_pin, int tx_pib, bool invert = false);
    ~ArduinoPort();
protected:

    virtual void _open();
    virtual void _close();
    virtual int _read(bool &nothing_to_read, bool &error);
	virtual bool is_open();

private:
    HardwareSerial& serial;
    int rx_pin;
    int tx_pin;
    bool invert;
    bool open;
};

#endif // LINUX_PORT_H