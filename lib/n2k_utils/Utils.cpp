#include "Utils.h"
#include "errno.h"
#include <time.h>
#include <math.h>
#include <string.h>
#include <cstdio>
#include <unistd.h>
#ifndef ESP32_ARCH
#include <sys/sysinfo.h>
#endif

unsigned long check_elapsed(ulong time, ulong &last_time, ulong period)
{
  ulong dT = time - last_time;
  if (dT>=period || dT<0)
  {
    last_time = time;
    return dT;
  }
  return 0;
}


bool startswith(const char* str_to_find, const char* str)
{
  int l_str = strlen(str);
  int l_find = strlen(str_to_find);
  if (l_find<=l_str)
  {
    for (int i = 0; i<l_find; i++)
    {
      if (str[i]!=str_to_find[i]) return false;
    }
    return true;
  }
  return false;
}

ulong _millis(void)
{
  #ifndef ESP32_ARCH
  long            ms; // Milliseconds
  time_t          s;  // Seconds
  struct timespec spec;

  clock_gettime(CLOCK_REALTIME, &spec);

  s  = spec.tv_sec;
  ms = round(spec.tv_nsec / 1.0e6); // Convert nanoseconds to milliseconds
  if (ms > 999) {
      s++;
      ms = 0;
  }

  return s * 1000 + ms;
  #else
  return millis();
  #endif
}

int msleep(long msec)
{
  #ifndef ESP32_ARCH
    struct timespec ts;
    int res;

    if (msec < 0)
    {
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    return res;
    #else
    delay(msec);
    return 0;
    #endif
}

unsigned long get_free_mem()
{
    #ifdef ESP32_ARCH
    return ESP.getFreeHeap();
    #else
    struct sysinfo info;
    sysinfo(&info);
    return info.freeram;
    #endif
}

void f1000(char* buf, unsigned long l, int& length)
{
  if (l<1000)
  {
    length += sprintf(buf + length, "%lu", l);
  }
  else
  {
    f1000(buf, l/1000, length);
    length += sprintf(buf + length, ",%03lu", l%1000);
  }
}

void format_thousands_sep(char* final, long toBeFormatted)
{
  int l = 0;
  f1000(final, toBeFormatted, l);
  return;
}

int indexOf(const char* haystack, const char* needle)
{
  const char* c1 = strstr(haystack, needle);
  if (c1)
  {
    return strlen(haystack) - strlen(c1);
  }
  else
  {
    return -1;
  }
}


char * replace(char const * const original, char const * const pattern, char const * const replacement, bool first) {
  size_t const replen = strlen(replacement);
  size_t const patlen = strlen(pattern);
  size_t const orilen = strlen(original);

  size_t patcnt = 0;
  const char * oriptr;
  const char * patloc;

  // find how many times the pattern occurs in the original string
  if (first) {
    patcnt += strstr(original, pattern)?1:0;
  } else {
    for (oriptr = original; (patloc = strstr(oriptr, pattern)); oriptr = patloc + patlen) {
      patcnt++;
    }
  }

  // allocate memory for the new string
  size_t const retlen = orilen + patcnt * (replen - patlen);
  char * const returned = (char *) malloc( sizeof(char) * (retlen + 1) );

  if (returned != NULL)
  {
    // copy the original string,
    // replacing all the instances of the pattern
    char * retptr = returned;
    for (oriptr = original; (patloc = strstr(oriptr, pattern)); oriptr = patloc + patlen)
    {
      size_t const skplen = patloc - oriptr;
      // copy the section until the occurence of the pattern
      strncpy(retptr, oriptr, skplen);
      retptr += skplen;
      // copy the replacement
      strncpy(retptr, replacement, replen);
      retptr += replen;
    }
    // copy the rest of the string.
    strcpy(retptr, oriptr);

    return returned;
  } else {
    return NULL;
  }
}

char *replace_and_free(char *orig, const char *pattern, const char *new_string, bool first)
{
    char *c = replace(orig, pattern, new_string, first);
    free(orig);
    return c;
}

// To store number of days in all months from January to Dec.
const int monthDays[12] = {31, 28, 31, 30, 31, 30,
                           31, 31, 30, 31, 30, 31};

int countLeapYears(int year, int month)
{
    // Check if the current year needs to be considered
    // for the count of leap years or not
    if (month <= 2)
        year--;

    // An year is a leap year if it is a multiple of 4,
    // multiple of 400 and not a multiple of 100.
    return year / 4 - year / 100 + year / 400;
}

int getDaysSince1970(int y, int m, int d) {
    // COUNT TOTAL NUMBER OF DAYS BEFORE FIRST DATE 'dt1'

    // initialize count using years and day
    long int n1 = 1970 * 365 + 1;

    // Since every leap year is of 366 days,
    // Add a day for every leap year
    n1 += countLeapYears(1970, 1);

    // SIMILARLY, COUNT TOTAL NUMBER OF DAYS BEFORE 'dt2'

    long int n2 = y * 365 + d;
    for (int i = 0; i < m - 1; i++)
        n2 += monthDays[i];
    n2 += countLeapYears(y, m);

    // return difference between two counts
    return (n2 - n1);
}

const char* time_to_ISO(time_t t, int millis)
{
  static char buf[32];
  strftime(buf, sizeof "2011-10-08T07:07:09.000Z", "%FT%T", gmtime(&t));
  sprintf(buf + 19, ".%03dZ", millis);
  return buf;
}

bool array_contains(short test, short* int_set, int set_size) {
    for (int i=0; i<set_size; i++) {
        if (test==int_set[i]) return true;
    }
    return false;
}

N2KSid::N2KSid(): sid(0) {}

unsigned char N2KSid::getCurrent()
{
  return sid;
}

unsigned char N2KSid::getNew()
{
  sid = (sid+1) % 253;
  return sid;
}

