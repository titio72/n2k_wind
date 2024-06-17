#ifdef ESP32_ARCH
#include <Arduino.h>
#include "ArduinoPort.h"
#include "Log.h"

ArduinoPort::ArduinoPort(HardwareSerial& s, int rx, int tx, bool _invert): serial(s), rx_pin(rx), tx_pin(tx), open(false), invert(_invert)
{
}

ArduinoPort::~ArduinoPort()
{
    serial.end();
}

void ArduinoPort::_open()
{
    Log::trace("[PORT] Opening serial speed {%d} rx {%d} tx {%d} invert {%d}\n", speed, rx_pin, tx_pin, invert);
    serial.begin(speed, SERIAL_8N1, rx_pin, tx_pin, invert);
    open = true;
    Log::trace("[PORT] Serial opened at {%d} BPS\n", speed);
}

void ArduinoPort::_close()
{
    Log::trace("[PORT] Closing serial port\n");
    serial.end();
    open = false;
    Log::trace("[PORT] Serial port closed\n");
}

int ArduinoPort::_read(bool &nothing_to_read, bool &error)
{
    error = false;
    if (serial.available()>0)
    {
        nothing_to_read = false;
        return serial.read();
    }
    else
    {
        nothing_to_read = true;
        return -1;
    }
}

bool ArduinoPort::is_open()
{
    return open;
}
#endif