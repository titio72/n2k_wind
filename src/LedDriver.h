#ifndef _LED_DRIVER_H
#define _LED_DRIVER_H

class LedDriver
{
public:
    LedDriver();
    virtual ~LedDriver();

    void setup();
    void loop(unsigned long time);

    void set_blue(bool blue);
    void set_error(int error);
    void set_calibration(bool cal);

private:
    bool blue;
    bool calib;
    int error;
};



#endif
