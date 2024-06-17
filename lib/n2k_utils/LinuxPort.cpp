#ifndef ESP32_ARCH
#include "Log.h"
#include "Utils.h"
#include "LinuxPort.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <errno.h>

LinuxPort::LinuxPort(const char* p_name): tty_fd(0)
{
    strcpy(port_name, p_name);
}

LinuxPort::~LinuxPort()
{
    if (tty_fd) ::close(tty_fd);
}

void LinuxPort::set_port_name(const char* pname)
{
    strcpy(port_name, p_name);
}

int fd_set_blocking(int fd, int blocking)
{
    // Save the current flags
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return 0;

    if (blocking)
        flags &= ~O_NONBLOCK;
    else
        flags |= O_NONBLOCK;
    return fcntl(fd, F_SETFL, flags) != -1;
}

void LinuxPort::_open()
{
    if (tty_fd==0)
	{
		struct termios tio;

		memset(&tio, 0, sizeof(tio));
		tio.c_iflag = 0;
		tio.c_oflag = 0;
		tio.c_cflag = CS8 | CREAD | CLOCAL; // 8n1, see termios.h for more information
		tio.c_lflag = 0;
		tio.c_cc[VMIN] = 1;
		tio.c_cc[VTIME] = 5;

		Log::trace("[GPS] Opening port {%s} at {%d} BPS\n", port, speed);

		tty_fd = ::open(port, O_RDONLY | O_NONBLOCK); // O_NONBLOCK might override VMIN and VTIME, so read() may return immediately.
		Log::trace("Err opening port {%d} {%s} {%d} {%s}\n", tty_fd, port, errno, strerror(errno));
		if (tty_fd > 0)
		{
			speed_t bps;
			switch (speed)
			{
				case 4800: bps = B4800; break;
				case 9600: bps = B9600; break;
				case 19200: bps = B19200; break;
				case 38400: bps = B38400; break;
				case 57600: bps = B57600; break;
				case 115200: bps = B115200; break;
				default:
				bps = B38400;
			}
			fd_set_blocking(tty_fd, 0);
			Log::trace("Err opening port {%s} {%d} {%s}\n", port, errno, strerror(errno));
			cfsetospeed(&tio, bps);
			Log::trace("Err opening port {%s} {%d} {%s}\n", port, errno, strerror(errno));
			cfsetispeed(&tio, bps);
			Log::trace("Err opening port {%s} {%d} {%s}\n", port, errno, strerror(errno));
			tcsetattr(tty_fd, TCSANOW, &tio);
			Log::trace("Err opening port {%s} {%d} {%s}\n", port, errno, strerror(errno));
		}
	}
}

void LinuxPort::_close()
{
    if (tty_fd) ::close(tty_fd);
    tty_fd = 0;
}

int LinuxPort::_read(bool &nothing, bool& error)
{
    unsigned char c;
    ssize_t bread = read(tty_fd, &c, 1);
    nothing = (errno==11);
    error = errno!=11 && errno!=OK;
    return c;
}

bool LinuxPort::is_open()
{
    return tty_fd;
}
#endif
