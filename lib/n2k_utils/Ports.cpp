#include "Ports.h"
#include "Log.h"
#include "Utils.h"

Port::Port(): bytes(0), listener(NULL)
{
}

Port::~Port()
{
}

void Port::set_handler(PortListener* l)
{
	listener = l;
}

int Port::process_char(unsigned char c)
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
			listener->on_partial(read_buffer);
		}
	}
	else if (pos != 0)
	{
		if (listener)
		{
			listener->on_line_read(read_buffer);
		}
		if (trace) {
			Log::trace("[PORT] Read {%s}\n", read_buffer);
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
	return 1;
}

void Port::listen(uint ms)
{
	static unsigned int last_speed = speed;

	unsigned char c;
	ulong t0 = _millis();

	if (last_speed != speed && is_open())
	{
		Log::trace("[PORT] Resetting speed {%d}\n", speed);
		close();
		last_speed = speed;
	}

	while (!is_open() && (_millis() - t0) < ms)
	{
		if (!open())
		{
			sleep(250);
		}
	}

	if (is_open())
	{
		bool stop = false;
		while (!stop)
		{
			bool error = false;
			bool nothing_to_read = false;
			int c = _read(nothing_to_read, error);
			if (!nothing_to_read && !error)
			{
				bytes++;
				process_char(c);
				stop = ((_millis() - t0) > ms);
			}
			else if (nothing_to_read)
			{
				// nothing to read
				return;
			}
			else
			{
				//Log::trace("Err reading port {%s} {%d} {%s}\n", port, errno, strerror(errno));
				close();
				return;
			}
		}
	}
}
