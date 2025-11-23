// CommandHandler.h
#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <Arduino.h>

enum class CommandResult : uint8_t
{
    SUCCESS = 0,
    INVALID_FORMAT = 1,
    MISSING_INPUT = 2,
    OUT_OF_RANGE = 3,
    INTERNAL_ERROR = 4,
    BUFFER_OVERFLOW = 5,
    WRITE_ERROR = 6,
    UNKNOWN_COMMAND = 7
};

class Calibration;
class Conf;
class BLEWind;

class CommandHandler
{
public:
    CommandHandler(Conf &conf, Calibration &cal, BLEWind &ble);

    CommandResult exec_command(const char *value);
    
private:
    Conf &conf;
    Calibration &auto_calibration;
    BLEWind &ble;
};
#endif