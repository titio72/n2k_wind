#include "Ports.h"
#include "Log.h"
#include "Utils.h"
#include <Arduino.h>

Port::Port(const char *name): bytes(0), listener(NULL), pos(0), last_speed(DEFAULT_PORT_SPEED), speed(DEFAULT_PORT_SPEED), last_open_try(0)
{
	strcpy(port_name, name);
}

Port::~Port()
{
}

void Port::set_handler(PortListener* l)
{
	listener = l;
}

int Port::process_char(char c)
{
	int res = 0;
	if (c != 10 && c != 13)
	{
		read_buffer[pos] = c;
		pos++;
		pos %= PORT_BUFFER_SIZE; // avoid buffer overrun
		read_buffer[pos] = 0;
		if (listener)
		{
			listener->on_partial_x(read_buffer, pos);
			listener->on_partial(read_buffer); // to be deprecated
		}
	}
	else if (pos != 0)
	{
		if (listener)
		{
			//Serial.printf("%s\n", read_buffer);
			listener->on_line_read(read_buffer);
		}
		if (trace) {
			Log::tracex("PORT", "Read", "buffer {%s}", read_buffer);
		}
		pos = 0;
		res = 1;
	}
	read_buffer[pos] = 0;
	return res;
}

void Port::close()
{
	_close();
}

int Port::open()
{
	_open();
	return is_open();
}

void Port::listen(uint ms)
{
	unsigned long t0 = _millis();

	if (last_speed != speed && is_open())
	{
		Log::tracex("PORT", "Resetting speed", "name {%s} new speed {%d} old speed {%d}", port_name, speed, last_speed);
		close();
		last_speed = speed;
	}

	if (!is_open() && check_elapsed(t0, last_open_try, 1000))
	{
		open();
	}

	if (!is_open())
	{
		return;
	}

	while ((_millis() - t0) < ms)
	{
		bool error = false;
		bool nothing_to_read = false;
		int c = _read(nothing_to_read, error);
		if (!nothing_to_read && !error)
		{
			bytes++;
			process_char(c);
		}
		else if (nothing_to_read)
		{
			// nothing to read
			return;
		}
		else
		{
			//Log::tracex("PORT", "Err reading", "port {%s} {%d} {%s}\n", port, errno, strerror(errno));
			close();
			return;
		}
	}
}
