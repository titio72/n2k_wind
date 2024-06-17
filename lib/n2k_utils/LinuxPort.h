#ifndef LINUXPORT_H_
#define LINUXPORT_H_

#include "Ports.h"

class LinuxPort: public Port
{

public:
    LinuxPort(const char* port_pathd = "/dev/ttyUSB0");
    void set_port_name(const char* port_pathd);
protected:

    virtual void _open();
    virtual void _close();
    virtual int _read(bool &nothing_to_read, bool &error);
	virtual bool is_open();

private:
    int tty_fd;
    char port_name[32];

};



#endif // LINUX_PORT_H