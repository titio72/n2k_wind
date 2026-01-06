// CommandHandler.h
#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <stdint.h>

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
struct configuration;
class ConfPersistence;
class IBLEWind;

class CommandHandler
{
public:
    CommandHandler(configuration &conf, ConfPersistence& confPersistence, Calibration &cal, IBLEWind &ble);

    CommandResult exec_command(const char *value);
    
private:
    ConfPersistence &confPersistence;
    configuration &conf;
    Calibration &calibration;
    IBLEWind &ble;
};
#endif