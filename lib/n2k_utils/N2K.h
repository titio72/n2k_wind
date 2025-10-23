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

class N2K {

    public:
        static N2K* get_instance(void (*_MsgHandler)(const tN2kMsg &N2kMsg), void (*_src_handler)(const unsigned char old_s, const unsigned char new_s));

        virtual ~N2K();

        void setup();

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
};

#endif
