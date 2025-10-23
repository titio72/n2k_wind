#include "Ports.h"
#include "Log.h"
#include <Arduino.h>
#include <SoftwareSerial.h>
#include <HardwareSerial.h>

#define LOG_PORT_PREFIX "PORT"

template <typename T> class ArduinoPort: public Port
{
public:
    ArduinoPort(const char* name, T& serial, int rx_pin, int tx_pib, bool invert = false);
    ArduinoPort(const char* name, T& serial, unsigned int bps, int rx, int tx, bool _invert = false);
    ~ArduinoPort();
    Stream& get_stream() { return serial; }

protected:

    virtual void _open();
    virtual void _close();
    virtual int _read(bool &nothing_to_read, bool &error);
	virtual bool is_open();

private:
    T& serial;
    int rx_pin;
    int tx_pin;
    bool invert;
    bool open;
};

template <typename T> ArduinoPort<T>::ArduinoPort(const char* name, T& s, int rx, int tx, bool _invert): Port(name), serial(s), rx_pin(rx), tx_pin(tx), open(false), invert(_invert)
{
}

template <typename T> ArduinoPort<T>::ArduinoPort(const char* name, T& s, unsigned int bps, int rx, int tx, bool _invert): Port(name), serial(s), rx_pin(rx), tx_pin(tx), open(false), invert(_invert)
{
    speed = bps;
}

template <typename T> ArduinoPort<T>::~ArduinoPort()
{
    serial.end();
}

template <> void ArduinoPort<HardwareSerial>::_open()
{
    Log::tracex(LOG_PORT_PREFIX, "Open serial", "type {HW} name {%s} speed {%d BPS} RX {%d} TX {%d} invert {%d}", port_name, speed, rx_pin, tx_pin, invert);
    serial.begin(speed, SERIAL_8N1, rx_pin, tx_pin, invert);
    open = true;
}

template <> void ArduinoPort<SoftwareSerial>::_open()
{
    Log::tracex(LOG_PORT_PREFIX, "Open serial", "type {SW} name {%s} speed {%d BPS} RX {%d} TX {%d} invert {%d}", port_name, speed, rx_pin, tx_pin, invert);
    serial.begin(speed, EspSoftwareSerial::SWSERIAL_8N1, rx_pin, tx_pin, invert);
    open = true;
}

template <typename T> void ArduinoPort<T>::_close()
{
    Log::tracex(LOG_PORT_PREFIX, "Close serial", "name {%s}", port_name);
    serial.end();
    open = false;
}

template <typename T> int ArduinoPort<T>::_read(bool &nothing_to_read, bool &error)
{
    error = false;
    if (serial.available()>0)
    {
        return serial.read();
    }
    else
    {
        nothing_to_read = true;
        return -1;
    }
}

template <typename T> bool ArduinoPort<T>::is_open()
{
    return open;
}

