/*
 * Log.cpp
 *
 *  Created on: Mar 26, 2019
 *      Author: aboni
 */
#ifdef ESP32_ARCH
#include <Arduino.h>
#endif

#include "Log.h"
#include <stdio.h>
#include <time.h>
#include <stdarg.h>

#define MAX_TRACE_SIZE 1024

static bool _debug = false;

static char outbfr[MAX_TRACE_SIZE];

inline bool can_trace()
{
#ifdef ESP32_ARCH
	return Serial.availableForWrite();
#else
	return true;
#endif
}

const char *_gettime()
{
	static char _buffer[80];
	time_t rawtime;
	struct tm *timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(_buffer, 80, "%T", timeinfo);
	return _buffer;
}

void _trace(const char *text)
{
#ifdef ESP32_ARCH
	Serial.print(text);
#else
	printf("%s", text);
	FILE *f = fopen("/var/log/nmea.log", "a+");
	if (f == NULL)
	{
		f = fopen("./nmea.log", "a+");
	}
	if (f)
	{
		fprintf(f, "%s %s\n", _gettime(), text);
		fclose(f);
	}
#endif
}

void Log::setdebug()
{
	_debug = true;
}

void Log::debug(const char *text, ...)
{
	if (_debug && can_trace())
	{
		va_list args;
		va_start(args, text);
		vsnprintf(outbfr, MAX_TRACE_SIZE, text, args);
		va_end(args);
		_trace(outbfr);
	}
}

void Log::trace(const char *text, ...)
{
	if (can_trace())
	{
		va_list args;
		va_start(args, text);
		vsnprintf(outbfr, MAX_TRACE_SIZE, text, args);
		va_end(args);
		_trace(outbfr);
	}
}

void Log::debugx(const char *module, const char *action, const char *text, ...)
{
	if (_debug && can_trace())
	{
		int l = snprintf(outbfr, MAX_TRACE_SIZE - 2, "[%s] %s: ", module, action);
		char* _outbfr = (outbfr + l);
		va_list args;
		va_start(args, text);
		int l1 = vsnprintf(_outbfr, MAX_TRACE_SIZE - 2 - l, text, args);
		va_end(args);
		_outbfr[l1] = '\n';
		_outbfr[l1+1] = 0;
		_trace(outbfr);
	}
}

void Log::tracex(const char *module, const char *action, const char *text, ...)
{
	if (can_trace())
	{
		int l = snprintf(outbfr, MAX_TRACE_SIZE - 2, "[%s] %s: ", module, action);
		char* _outbfr = (outbfr + l);
		va_list args;
		va_start(args, text);
		int l1 = vsnprintf(_outbfr, MAX_TRACE_SIZE -2 - l, text, args);
		va_end(args);
		_outbfr[l1] = '\n';
		_outbfr[l1+1] = 0;
		_trace(outbfr);
	}
}

void Log::tracex(const char *module, const char *action)
{
	if (can_trace())
	{
		int l = snprintf(outbfr, MAX_TRACE_SIZE - 2, "[%s] %s", module, action);
		outbfr[l] = '\n';
		outbfr[l+1] = 0;
		_trace(outbfr);
	}
}

