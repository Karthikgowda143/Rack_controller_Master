#ifndef _CLOCK_
#define _CLOCK_

#include <time.h>
#include <Wire.h>
#include <PCF8563.h>


extern time_t currentEpochTime;


void rtc_Init(void);
Time rtc_GetTime(void);
void rtc_SetTime(uint8_t year_since_2000, uint8_t month, uint8_t date, uint8_t hour, uint8_t minute, uint8_t second);
void printInternalRTC(void);
void _rtc_SyncESPInternalRTC(void);
struct tm getInternalRTC(void);
time_t getEpochTime(void);
String getFormattedTime();

#endif
