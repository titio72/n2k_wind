#ifndef N2K_H
#define N2K_H

#include <N2kMessages.h>
#include "Utils.h"

class tNMEA2000;

class N2KStats
{
public:
    unsigned long recv = 0;
    unsigned long sent = 0;
    unsigned long fail = 0;
    unsigned char canbus = 0;
    void dump();
    void reset();
};

struct n2k_device_info
{
    std::string ModelSerialCode = "0.0.1";
    unsigned short ProductCode = 100;
    std::string ModelID = "AB";
    std::string SwCode = "AB 0.0.1";
    std::string ModelVersion = "0001";

    unsigned long UniqueNumber = 1;     // Unique number. Use e.g. Serial number.
    unsigned char DeviceFunction = 145; // Device function=Analog to NMEA 2000 Gateway. See codes on http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
    unsigned char DeviceClass = 60;     // Device class=Inter/Intranetwork Device. See codes on  http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
    uint16_t ManufacturerCode = 2046;   // Just choosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
};

class N2K {

    public:
        static N2K* get_instance(void (*_MsgHandler)(const tN2kMsg &N2kMsg), void (*_src_handler)(const unsigned char old_s, const unsigned char new_s));

        virtual ~N2K();

        void setup(n2k_device_info dvc);

        void loop(unsigned long time);

        bool send_msg(const tN2kMsg &N2kMsg);

        bool is_initialized();

        // used only on linux
        void set_can_socket_name(const char* name);

        N2KStats getStats();

        unsigned char get_source();
        void set_desired_source(unsigned char src);

        void add_pgn(unsigned long pgns);

        static void set_sent_message_callback(void (*_MsgHandler)(const tN2kMsg &N2kMsg, bool success));

    private:
        N2K();
        tNMEA2000* NMEA2000;
        char socket_name[32];
        unsigned char desired_source;
        unsigned long* pgns;
        int n_pgns;
        n2k_device_info device_info;

};

#endif
