#include "CommandHandler.h"
#include <Arduino.h>
#include <Log.h>
#include "DataAndConf.h"
#include "SinCosDecoder.h"
#include "WindSpeed.h"
#include "WindDirection.h"
#include "AutoCalibration.h"

CommandResult apply_calibration(Range &sin, Range& cos, Conf &conf)
{
    if (cos.is_valid() && sin.is_valid())
    {
        conf.cos_range.set(cos);
        conf.sin_range.set(sin);
        return conf.write() ? CommandResult::SUCCESS : CommandResult::WRITE_ERROR;
    }
    else
    {
        Log::trace("[CMD] Manual calibration invalid\n");
        return CommandResult::INVALID_FORMAT;
    }
}

CommandHandler::CommandHandler(Conf &c, AutoCalibration &cal) : conf(c), auto_calibration(cal),
                                                     last_BT_is_alive(0)
{
}

CommandResult CommandHandler::on_command(int handle, const char *value)
{
    if (value && value[0])
    {
        const char command = value[0];
        const char *command_value = (value + sizeof(char)); // skip the first char (which is the command code)
        switch (command)
        {
        case 'K': // Set speed adj
            return command_set_speed_adj(command_value);
        case 'O': // Set offset
            return command_set_offset(command_value);
        case 'S': // Set manual calibration
            return command_set_calibration(command_value);
        case 'A': // Abort calibration
            return command_abort_calibration();
        case 'R': // Finalize calibration
            return command_finalize_calibration();
        case 'X': // Calibration factory reset
            return command_factory_reset();
        case 'H': // Heartbeat
            return command_heartbeat(); 
        case 'W': // change LPF alpha for the speed smoothing
            return command_set_speed_smoothing(command_value);
        case 'Q': // change LPF alpha for the direction smoothing
            return command_set_angle_smoothing(command_value);
        case 'P': // Start auto calibration
            return command_toggle_autocalib();
        case 'T': // auto calibration threshold
            return command_set_auto_calibration_threshold(command_value);
        case 'D':
            return command_toggle_debug();
        default:
            Log::trace("[CAL] Unrecognized command {%s}\n", value);
            return CommandResult::UNKNOWN_COMMAND;
        }
    }
    else
    {
        return CommandResult::MISSING_INPUT;
    }
}

unsigned long CommandHandler::get_last_BT_activity()
{
    return last_BT_is_alive;
}

CommandResult CommandHandler::command_heartbeat()
{
    last_BT_is_alive = millis();
    return CommandResult::SUCCESS;
}

CommandResult CommandHandler::command_set_speed_adj(const char *command_value)
{
    Log::trace("[CMD] Setting speed adjustment {%s}\n", command_value);
    if (command_value[0])
    {
        int32_t adj = 0;
        if (atoi_x(adj, command_value))
        {
            uint32_t uiAdj = adj & 0xFF; // trim to 0..255
            Log::trace("[CMD] New speed adjustment {%d}\n", uiAdj);
            conf.speed_adjustment = uiAdj;
            return conf.write() ? CommandResult::SUCCESS : CommandResult::WRITE_ERROR;
        }
        else
        {
            Log::trace("[CMD] Invalid speed adjustment\n");
            return CommandResult::INVALID_FORMAT;
        }
    }
    else
    {
        Log::trace("[CMD] Missing speed adjustment\n");
        return CommandResult::MISSING_INPUT;
    }
}

CommandResult CommandHandler::command_set_offset(const char *command_value)
{
    Log::trace("[CMD] Setting offset {%s}\n", command_value);
    if (command_value[0])
    {
        int32_t offset = 0;
        if (atoi_x(offset, command_value))
        {
            Log::trace("[CMD] New offset {%d}\n", offset);
            conf.offset = offset;
            return conf.write() ? CommandResult::SUCCESS : CommandResult::WRITE_ERROR;
        }
        else
        {
            Log::trace("[CMD] Invalid offset\n");
            return CommandResult::INVALID_FORMAT;
        }
    }
    else
    {
        Log::trace("[CMD] Invalid offset\n");
        return CommandResult::MISSING_INPUT;
    }
}

CommandResult CommandHandler::command_set_calibration(const char *command_value)
{
    Log::trace("[CMD] Setting manual calibration {%s}\n", command_value);
    int i_tok = 0;
    int32_t vv[] = {-1, -1, -1, -1};
    bool do_write = false;
    char s[256];
    strcpy(s, command_value);
    char *t, *p;
    for (t = mystrtok(&p, s, '|'); t && i_tok < 4; t = mystrtok(&p, 0, '|'))
    {
        if (strlen(t))
        {
            parse_value(vv[i_tok], t, MAX_ADC_VALUE);
        }
        i_tok++;
    }
    Log::trace("[CMD] Read manual cal values {%d %d %d %d}\n", vv[0], vv[1], vv[2], vv[3]);
    if (vv[0] >= 0 && vv[1] >= 0 && vv[2] >= 0 && vv[3] >= 0)
    {
        Range s_range((uint16_t)vv[0], (uint16_t)vv[1], RANGE_DEFAULT_VALID);
        Range c_range((uint16_t)vv[2], (uint16_t)vv[3], RANGE_DEFAULT_VALID);
        return apply_calibration(s_range, c_range, conf);
    }
    else
    {
        Log::trace("[CMD] Manual calibration invalid\n");
        return CommandResult::INVALID_FORMAT;
    }
}

CommandResult CommandHandler::command_finalize_calibration()
{
    Log::trace("[CMD] finalize calibration\n");
    auto_calibration.apply_calibration();
    return CommandResult::SUCCESS;
}

CommandResult CommandHandler::command_factory_reset()
{
    Log::trace("[CMD] Reset calibration to default\n");
    conf.offset = 0;
    Range def_range;
    return apply_calibration(def_range, def_range, conf);
}

CommandResult CommandHandler::command_abort_calibration()
{
    Log::trace("[CMD] Abort calibration\n");
    auto_calibration.reset();
    return CommandResult::SUCCESS;
}

CommandResult CommandHandler::command_set_speed_smoothing(const char *command_value)
{
    Log::trace("[CAL] Setting speed smoothing {%s}\n", command_value);
    if (command_value[0])
    {
        int32_t smoothing = 0;
        if (atoi_x(smoothing, command_value))
        {
            uint32_t uSmoothing = smoothing & 0xFF; // trim to 0..255
            uSmoothing = (uSmoothing<SMOOTHING_ALPHA_MIN)?SMOOTHING_ALPHA_MIN:uSmoothing;
            uSmoothing = (uSmoothing>SMOOTHING_ALPHA_MAX)?SMOOTHING_ALPHA_MAX:uSmoothing;
            Log::trace("[CAL] New speed smoothing {%d} alpha {%.2f}\n", uSmoothing, conf.get_speed_smoothing_factor());
            conf.speed_smoothing = uSmoothing;
            return conf.write() ? CommandResult::SUCCESS : CommandResult::WRITE_ERROR;
        }
        else
        {
            Log::trace("[CAL] Invalid speed smoothing\n");
            return CommandResult::INVALID_FORMAT;
        }
    }
    else
    {
        Log::trace("[CAL] Invalid speed smoothing\n");
        return CommandResult::MISSING_INPUT;
    }
}

CommandResult CommandHandler::command_set_angle_smoothing(const char *command_value)
{
    Log::trace("[CAL] Setting angle smoothing {%s}\n", command_value);
    if (command_value[0])
    {
        int32_t smoothing = 0;
        if (atoi_x(smoothing, command_value))
        {
            uint32_t uSmoothing = smoothing & 0xFF; // trim to 0..255
            uSmoothing = (uSmoothing<SMOOTHING_ALPHA_MIN)?SMOOTHING_ALPHA_MIN:uSmoothing;
            uSmoothing = (uSmoothing>SMOOTHING_ALPHA_MAX)?SMOOTHING_ALPHA_MAX:uSmoothing;
            conf.angle_smoothing = uSmoothing;
            Log::trace("[CAL] New angle smoothing {%d} alpha {%.2f}\n", uSmoothing, conf.get_angle_smoothing_factor());
            return conf.write() ? CommandResult::SUCCESS : CommandResult::WRITE_ERROR;
        }
        else
        {
            Log::trace("[CAL] Invalid angle smoothing\n");
            return CommandResult::INVALID_FORMAT;
        }
    }
    else
    {
        Log::trace("[CAL] Invalid angle smoothing\n");
        return CommandResult::MISSING_INPUT;
    }
}

CommandResult CommandHandler::command_set_auto_calibration_threshold(const char *command_value)
{
    Log::trace("[CAL] Setting autocalibration threshold {%s}\n", command_value);
    if (command_value[0])
    {
        int32_t threshold = 0;
        if (atoi_x(threshold, command_value))
        {
            uint8_t uT = threshold & 0xFF; // trim to 0..255
            uT = (uT<AUTO_CALIB_THRESHOLD_MIN)?AUTO_CALIB_THRESHOLD_MIN:uT;
            uT = (uT>AUTO_CALIB_THRESHOLD_MAX)?AUTO_CALIB_THRESHOLD_MAX:uT;
            Log::trace("[CAL] New auto calibration threshold {%d} {%.2f}\n", uT, conf.get_calibration_threshold_factor());
            conf.calibration_score_threshold = uT;
            return conf.write() ? CommandResult::SUCCESS : CommandResult::WRITE_ERROR;
        }
        else
        {
            Log::trace("[CAL] Invalid auto calibration threshold\n");
            return CommandResult::INVALID_FORMAT;
        }
    }
    else
    {
        Log::trace("[CAL] Invalid auto calibration threshold\n");
        return CommandResult::MISSING_INPUT;
    }
}

CommandResult CommandHandler::command_toggle_autocalib()
{
    Log::trace("[CAL] Toggle auto calibration {%s}\n", auto_calibration.is_enabled() ? "OFF" : "ON");
    if (auto_calibration.is_enabled())
    {
        conf.auto_cal = 0;
        auto_calibration.enable(false);
    }
    else
    {
        conf.auto_cal = 1;
        auto_calibration.reset();
        auto_calibration.enable(true);
    }
    return conf.write() ? CommandResult::SUCCESS : CommandResult::WRITE_ERROR;
}

CommandResult CommandHandler::command_toggle_debug()
{
    Log::trace("[CAL] Toggle debug info on USB {%s}\n", conf.usb_tracing ? "OFF" : "ON");
    conf.usb_tracing = !conf.usb_tracing;
    if (conf.usb_tracing)
    {
        Log::enable();
    }
    else
    {
        Log::disable();
    }
    return conf.write() ? CommandResult::SUCCESS : CommandResult::WRITE_ERROR;
}
