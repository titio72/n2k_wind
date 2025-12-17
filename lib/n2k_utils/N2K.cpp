#ifndef NATIVE
#ifndef CAN_TX_PIN
#define CAN_TX_PIN GPIO_NUM_4
#endif
#ifndef CAN_RX_PIN
#define CAN_RX_PIN GPIO_NUM_5
#endif
#ifdef TWAI_CAN_DRIVER
#define N2K_CLASS_NAME "esp32xx"
#define N2K_CLASS tNMEA2000_esp32xx
#include <NMEA2000_esp32xx.h>
#else
#define N2K_CLASS_NAME "esp32"
#define N2K_CLASS tNMEA2000_esp32
#include <NMEA2000_esp32.h>
#endif
#else 
#ifdef SOCKET_CAN
#define N2K_CLASS_NAME "socket_can"
#define N2K_CLASS tNMEA2000_SocketCAN
#include <NMEA2000_SocketCAN.h>
#endif
#endif

#include <time.h>
#include <math.h>
#include <string.h>
#include "N2K.h"
#include "Utils.h"
#include "Log.h"
#include <NMEA2000.h>

#define N2K_LOG_TAG "N2k"

static bool static_initialized = false;
static N2K *instance = NULL;
static N2KStats stats;

static n2k_msg_handler _handler = nullptr;
static n2k_source_change_handler _source_handler = nullptr;
static n2k_sent_message_handler _sent_message_handler = nullptr;

N2K *N2K::get_instance(n2k_msg_handler _msg_handler, n2k_source_change_handler _src_handler)
{
    if (instance == NULL)
    {
        _handler = _msg_handler;
        _source_handler = _src_handler;
        instance = new N2K();
    }
    return instance;
}

void N2K::set_sent_message_callback(n2k_sent_message_handler _MsgHandler)
{
    _sent_message_handler = _MsgHandler;
}

N2K::N2K()
{
    NMEA2000 = nullptr;
    desired_source = N2K_SOURCE_DEFAULT;
    pgns.clear();
}

N2K::~N2K()
{
}

void private_message_handler(const tN2kMsg &N2kMsg)
{
    stats.recv++;
    if (_handler) _handler(N2kMsg);
}

unsigned char N2K::get_source()
{
    return NMEA2000->GetN2kSource();
}

void N2K::set_desired_source(unsigned char src)
{
    desired_source = src;
}

bool N2K::is_initialized()
{
    return static_initialized;
}

void N2K::loop(unsigned long time)
{
    if (is_initialized() && NMEA2000)
    {
        NMEA2000->ParseMessages();
        stats.canbus = NMEA2000->IsOpen() ? 1 : 0;
        unsigned char s = NMEA2000->GetN2kSource();
        if (s != desired_source)
        {
            // claimed new source
            Log::tracex(N2K_LOG_TAG, "Source claim", "old {%d} new {%d}", desired_source, s);
            unsigned char old_s = desired_source;
            desired_source = s;
            if (_source_handler) _source_handler(old_s, s);
        }
    }
}

void N2K::set_can_socket_name(const char *name)
{
    // bounded copy to avoid overflow
    strncpy(socket_name, name, sizeof(socket_name) - 1);
    socket_name[sizeof(socket_name) - 1] = '\0';
}

void N2K::add_pgn(unsigned long pgn)
{
    // Keep a simple vector of PGNs; we'll add a terminating 0 when passing to C API
    pgns.push_back(pgn);
}

#ifndef NATIVE
#define CREATE_NMEA \
Log::tracex(N2K_LOG_TAG, "Initializing N2K", "RX {%d} TX {%d} source {%d}", CAN_RX_PIN, CAN_TX_PIN, desired_source); \
NMEA2000 = new N2K_CLASS(CAN_TX_PIN, CAN_RX_PIN);
#else
#ifdef SOCKET_CAN
#define CREATE_NMEA \
Log::trace(N2K_LOG_TAG, "Initializing N2K", "socket {%s}", socket_name); \
NMEA2000 = new tNMEA2000_SocketCAN(socket_name);
#else
#define CREATE_NMEA \
NMEA2000 = nullptr;
#endif
#endif

void N2K::setup(n2k_device_info dvc)
{
    if (!is_initialized())
    {
        CREATE_NMEA
        if (NMEA2000)
        {
            NMEA2000->SetProductInformation(dvc.ModelSerialCode.c_str(), dvc.ProductCode, dvc.ModelID.c_str(), dvc.SwCode.c_str(), dvc.ModelVersion.c_str());
            NMEA2000->SetDeviceInformation(dvc.UniqueNumber, dvc.DeviceFunction, dvc.DeviceClass, dvc.ManufacturerCode);
            if (_handler)
            {
                NMEA2000->SetMsgHandler(private_message_handler);
            }
            NMEA2000->SetMode(tNMEA2000::N2km_NodeOnly, desired_source);
            NMEA2000->SetN2kCANSendFrameBufSize(1000);
            NMEA2000->EnableForward(false); // Disable all msg forwarding to USB (=Serial)
            if (!pgns.empty()) {
                // NMEA2000 expects an array terminated by 0 — create a temporary vector with a trailing 0
                std::vector<unsigned long> ext = pgns;
                ext.push_back(0);
                NMEA2000->ExtendTransmitMessages(ext.data());
            }
            int retry = 0;
            do {
                static_initialized = NMEA2000->Open();
                if (!static_initialized)
                {
                    retry++;
                    Log::tracex(N2K_LOG_TAG, "Failed N2K init", "Retry {%d}", retry);
                    msleep(1000);
                }
            } while (!static_initialized && retry < 5);
            Log::tracex(N2K_LOG_TAG, "initialized", "success {%s}", is_initialized() ? "OK" : "KO");
        }
    }
}

bool N2K::is_bus_connected()
{
    return NMEA2000 && NMEA2000->IsOpen();
}

bool N2K::send_msg(const tN2kMsg &N2kMsg)
{
    if (is_bus_connected() && NMEA2000)
    {
        if (_handler)
        {
            // notify internal listeners, that otherwise would not get the message
            _handler(N2kMsg);
        }
        bool res = NMEA2000->SendMsg(N2kMsg);
        if (res)
        {
            stats.sent++;
        }
        else
        {
            //Log::tracex(N2K_LOG_TAG, "Failed message", "PGN {%d}", N2kMsg.PGN);
            stats.fail++;
        }
        if (_sent_message_handler) _sent_message_handler(N2kMsg, res);
        return res;
    }
    else
    {
        return false;
    }
}

void N2KStats::dump()
{
    Log::tracex(N2K_LOG_TAG, "Stats", "bus {%d} tx {%d/%d} rx {%d}", canbus, sent, fail, recv);
}

void N2KStats::dump_and_reset()
{
    dump();
    reset();
}

void N2KStats::reset()
{
    recv = 0;
    sent = 0;
    fail = 0;
}

N2KStats N2K::getStats()
{
    return stats;
}
