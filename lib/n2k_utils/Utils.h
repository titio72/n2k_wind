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
const char* time_to_ISO(time_t t, int millis);
char* replace(char const * const original, char const * const pattern, char const * const replacement, bool first = false);
char *replace_and_free(char *orig, const char *pattern, const char *new_string, bool first);
int indexOf(const char* haystack, const char* needle);
bool array_contains(short test, short* int_set, int sz);
ulong _millis();
int msleep(long msec);
unsigned long get_free_mem();
unsigned long check_elapsed(ulong time, ulong &last_time, ulong period);
void format_thousands_sep(char* buffer, long l);

#endif
