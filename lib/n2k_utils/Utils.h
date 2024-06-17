#ifndef UTILS_H
#define UTILS_H

#define AB_AGENT \
void enable(); \
void disable(); \
bool is_enabled(); \
void loop(unsigned long time); \
void setup();


#ifdef ESP32_ARCH
#include <Arduino.h>
#endif

#include <stdlib.h>
#include <time.h>

class N2KSid
{
  public:
    N2KSid();
    unsigned char getNew();
    unsigned char getCurrent();
  private:
    unsigned char sid;
};

bool startswith(const char* str_to_find, const char* str);
int getDaysSince1970(int y, int m, int d);
char* replace(char const * const original, char const * const pattern, char const * const replacement, bool first = false);
char *replace_and_free(char *orig, const char *pattern, const char *new_string, bool first);
int indexOf(const char* haystack, const char* needle);
bool array_contains(short test, short* int_set, int sz);
ulong _millis();
int msleep(long msec);
unsigned long get_free_mem();

void format_thousands_sep(char* buffer, long l);

struct sat {
    int sat_id;
    int elev;
    int az;
    int db;
    int used;
};

#define MAX_USED_SATS_SIZE 24
#define MAX_SATS_SIZE 255

struct GSA {
    short valid =0;
    short nSat = 0;
    unsigned char fix = 0;
    short sats[MAX_USED_SATS_SIZE];
    float hdop = NAN;
    float vdop = NAN;
    float tdop = NAN;
    float pdop = NAN;
};

struct RMC {
    short valid = 0;
    float lat = NAN;
    float lon = NAN;

    short y = 0;
    short M = 0;
    short d = 0;
    short h = 0;
    short m = 0;
    short s = 0;
    short ms = 0;

    float cog = NAN;
    float sog = NAN;

    time_t unix_time = 0;
};

struct GSV {
  short nSat = 0;
  sat satellites[MAX_SATS_SIZE];
};

struct data {
  RMC rmc;
  GSA gsa;
  GSV gsv;
  double pressure = NAN;
  double humidity = NAN;
  double temperature = NAN;
  double temperature_el = NAN;
  double latitude = NAN;
  char latitude_NS = 'N';
  double longitude = NAN;
  char longitude_EW = 'E';
  double voltage = NAN;
};

#endif
