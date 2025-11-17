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

class AutoCalibration;
class Conf;

class CommandHandler
{
public:
    CommandHandler(Conf &conf, AutoCalibration &cal);

    CommandResult on_command(int handle, const char *value);

    unsigned long get_last_BT_activity();
    
private:
    Conf &conf;
    AutoCalibration &auto_calibration;
    unsigned long last_BT_is_alive;

    CommandResult command_set_speed_adj(const char *command_value);
    CommandResult command_set_offset(const char *command_value);
    CommandResult command_set_calibration(const char *command_value);
    CommandResult command_finalize_calibration();
    CommandResult command_abort_calibration();
    CommandResult command_set_speed_smoothing(const char *command_value);
    CommandResult command_set_angle_smoothing(const char *command_value);
    CommandResult command_set_auto_calibration_threshold(const char *command_value);
    CommandResult command_toggle_autocalib();
    CommandResult command_toggle_debug();
    CommandResult command_factory_reset();
    CommandResult command_heartbeat();
};
#endif