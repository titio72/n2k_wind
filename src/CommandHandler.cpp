#include <Arduino.h>
#include <Log.h>
#include "CommandHandler.h"
#include "DataAndConf.h"
#include "AutoCalibration.h"

struct CommandContext
{
    Conf &conf;
    AutoCalibration &cal;
    unsigned long last_BT_is_alive;
};

typedef CommandResult (*wind_command)(CommandContext &ctx, const char *command_value);
wind_command commands[256];

#pragma region commands
CommandResult apply_calibration(Range &sin, Range& cos, calibration_callback cback)
{
    if (cback && cback(sin, cos))
    {
        return CommandResult::SUCCESS;
    }
    else
    {
        Log::trace("[CMD] Manual calibration failed.\n");
        return CommandResult::INTERNAL_ERROR;
    }
}

CommandResult command_vane_type(CommandContext &ctx, const char* command_value)
{
    Log::trace("[CMD] Setting vane type {%s}\n", command_value);
    if (command_value[0])
    {
        int32_t vane = 0;
        if (atoi_x(vane, command_value))
        {
            if (vane==VANE_TYPE_ST50 || vane==VANE_TYPE_ST60)
            {
                Log::trace("[CMD] New vane type {%s}\n", vane==VANE_TYPE_ST50?"ST50":"ST60");
                ctx.conf.vane_type = vane;
                return ctx.conf.write() ? CommandResult::SUCCESS : CommandResult::WRITE_ERROR;
            }
        }
        Log::trace("[CMD] Invalid vane type\n");
        return CommandResult::INVALID_FORMAT;
    }
    else
    {
        Log::trace("[CMD] Invalid vane type\n");
        return CommandResult::MISSING_INPUT;
    }
}

CommandResult command_heartbeat(CommandContext &ctx, const char* command_value)
{
    ctx.last_BT_is_alive = millis();
    return CommandResult::SUCCESS;
}

CommandResult command_set_speed_adj(CommandContext &ctx, const char *command_value)
{
    Log::trace("[CMD] Setting speed adjustment {%s}\n", command_value);
    if (command_value[0])
    {
        int32_t adj = 0;
        if (atoi_x(adj, command_value))
        {
            uint32_t uiAdj = adj & 0xFF; // trim to 0..255
            Log::trace("[CMD] New speed adjustment {%d}\n", uiAdj);
            ctx.conf.speed_adjustment = uiAdj;
            return ctx.conf.write() ? CommandResult::SUCCESS : CommandResult::WRITE_ERROR;
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

CommandResult command_set_offset(CommandContext &ctx, const char *command_value)
{
    Log::trace("[CMD] Setting offset {%s}\n", command_value);
    if (command_value[0])
    {
        int32_t offset = 0;
        if (atoi_x(offset, command_value))
        {
            Log::trace("[CMD] New offset {%d}\n", offset);
            ctx.conf.offset = offset;
            return ctx.conf.write() ? CommandResult::SUCCESS : CommandResult::WRITE_ERROR;
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

CommandResult command_set_calibration(CommandContext &ctx, const char *command_value)
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
        return apply_calibration(s_range, c_range, ctx.cal.get_callback());
    }
    else
    {
        Log::trace("[CMD] Manual calibration invalid\n");
        return CommandResult::INVALID_FORMAT;
    }
}

CommandResult command_finalize_calibration(CommandContext &ctx, const char *command_value)
{
    Log::trace("[CMD] finalize calibration\n");
    ctx.cal.apply_calibration();
    return CommandResult::SUCCESS;
}

CommandResult command_factory_reset(CommandContext &ctx, const char *command_value)
{
    Log::trace("[CMD] Reset calibration to default\n");
    ctx.conf.offset = 0;
    Range def_range;
    return apply_calibration(def_range, def_range, ctx.cal.get_callback());
}

CommandResult command_abort_calibration(CommandContext &ctx, const char *command_value)
{
    Log::trace("[CMD] Abort calibration\n");
    ctx.cal.reset();
    return CommandResult::SUCCESS;
}

CommandResult command_set_speed_smoothing(CommandContext &ctx, const char *command_value)
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
            ctx.conf.speed_smoothing = uSmoothing;
            Log::trace("[CAL] New speed smoothing {%d} alpha {%.2f}\n", uSmoothing, ctx.conf.get_speed_smoothing_factor());
            return ctx.conf.write() ? CommandResult::SUCCESS : CommandResult::WRITE_ERROR;
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

CommandResult command_set_angle_smoothing(CommandContext &ctx, const char *command_value)
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
            ctx.conf.angle_smoothing = uSmoothing;
            Log::trace("[CAL] New angle smoothing {%d} alpha {%.2f}\n", uSmoothing, ctx.conf.get_angle_smoothing_factor());
            return ctx.conf.write() ? CommandResult::SUCCESS : CommandResult::WRITE_ERROR;
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

CommandResult command_set_auto_calibration_threshold(CommandContext &ctx, const char *command_value)
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
            Log::trace("[CAL] New auto calibration threshold {%d} {%.2f}\n", uT, ctx.conf.get_calibration_threshold_factor());
            ctx.conf.calibration_score_threshold = uT;
            return ctx.conf.write() ? CommandResult::SUCCESS : CommandResult::WRITE_ERROR;
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

CommandResult command_toggle_autocalib(CommandContext &ctx, const char *command_value)
{
    Log::trace("[CAL] Toggle auto calibration {%s}\n", ctx.cal.is_enabled() ? "OFF" : "ON");
    if (ctx.cal.is_enabled())
    {
        ctx.conf.auto_cal = 0;
        ctx.cal.enable(false);
    }
    else
    {
        ctx.conf.auto_cal = 1;
        ctx.cal.reset();
        ctx.cal.enable(true);
    }
    return ctx.conf.write() ? CommandResult::SUCCESS : CommandResult::WRITE_ERROR;
}

CommandResult command_toggle_debug(CommandContext &ctx, const char *command_value)
{
    Log::trace("[CAL] Toggle debug info on USB {%s}\n", ctx.conf.usb_tracing ? "OFF" : "ON");
    ctx.conf.usb_tracing = !ctx.conf.usb_tracing;
    if (ctx.conf.usb_tracing)
    {
        Log::enable();
    }
    else
    {
        Log::disable();
    }
    return ctx.conf.write() ? CommandResult::SUCCESS : CommandResult::WRITE_ERROR;
}
#pragma endregion

CommandHandler::CommandHandler(Conf &c, AutoCalibration &cal) : conf(c), auto_calibration(cal),
                                                     last_BT_is_alive(0)
{
    memset(commands, 0, sizeof(commands));
    commands['K'] = command_set_speed_adj; // Set speed adj
    commands['O'] = command_set_offset; // Set offset
    commands['S'] = command_set_calibration; // Set manual calibration
    commands['A'] = command_abort_calibration; // Abort calibration
    commands['R'] = command_finalize_calibration; // Finalize calibration
    commands['X'] = command_factory_reset; // Calibration factory reset
    commands['H'] = command_heartbeat; // Heartbeat
    commands['W'] = command_set_speed_smoothing; // change LPF alpha for the speed smoothing
    commands['Q'] = command_set_angle_smoothing; // change LPF alpha for the direction smoothing
    commands['P'] = command_toggle_autocalib; // Start auto calibration
    commands['T'] = command_set_auto_calibration_threshold; // auto calibration threshold
    commands['D'] = command_toggle_debug; // enable/disable USB tracing
    commands['V'] = command_vane_type; // send vane type (0=ST50, 1=ST60)
}

CommandResult CommandHandler::on_command(int handle, const char *value)
{
    if (value && value[0])
    {
        const char command = value[0];
        wind_command cmd = commands[command];
        if (cmd)
        {
            const char *command_value = (value + sizeof(char)); // skip the first char (which is the command code)
            CommandContext ctx = {conf, auto_calibration, last_BT_is_alive};
            return cmd(ctx, command_value);
        }
        else
        {
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
