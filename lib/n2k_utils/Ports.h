/*
 * Ports.h
 *
 *  Created on: Sep 29, 2018
 *      Author: aboni
 */

#ifndef PORTS_H_
#define PORTS_H_

#include <stdlib.h>

#define PORT_BUFFER_SIZE 1024

class PrivatePort;

class PortListener
{
public:
	virtual void on_line_read(const char* line) = 0;
	virtual void on_partial(const char* liine) = 0;
};

class Port {

public:
	virtual ~Port();

	void listen(unsigned int ms);
	void close();

	//void set_handler(int (*fun)(const char*));
	void set_handler(PortListener* listener);

	void debug(bool dbg=true) { trace = dbg; }

	void set_speed(unsigned int requested_speed) { speed = requested_speed; }

protected:
	Port();

	virtual void _open() = 0;
	virtual void _close() = 0;
	virtual int _read(bool &nothing_to_read, bool &error) = 0;
	virtual bool is_open() = 0;

	unsigned int speed = 38400;

private:
	int open();
	int process_char(unsigned char c);

	char read_buffer[PORT_BUFFER_SIZE];
	unsigned int pos = 0;

	bool trace = false;

	PortListener* listener;

	unsigned long bytes;
};

#endif // PORTS_H_
