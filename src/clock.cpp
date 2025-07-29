//#include "clock.h"
#include "main.h"

time_t currentEpochTime; // Global variable to hold the current epoch time

void _rtc_SyncESPInternalRTC(void);

static PCF8563 pcf;

void rtc_Init(void) {
    pcf.init();     //initialize the clock
    _rtc_SyncESPInternalRTC();
}

void rtc_SetTime(uint8_t year_since_2000, uint8_t month, uint8_t date, uint8_t hour, uint8_t minute, uint8_t second) {
    pcf.stopClock();                //stop the clock

    pcf.setYear(year_since_2000);   //set year
    pcf.setMonth(month);            //set month
    pcf.setDay(date);               //set date
    pcf.setHour(hour);              //set hour
    pcf.setMinut(minute);           //set minute
    pcf.setSecond(second);          //set second

    pcf.startClock();               //start the clock
    _rtc_SyncESPInternalRTC();
}

Time rtc_GetTime(void) {
    return pcf.getTime();
}

void _rtc_SyncESPInternalRTC(void) {
    Time time_info = rtc_GetTime();
    tm esp_time_info = {
        .tm_sec = time_info.second,
        .tm_min = time_info.minute,
        .tm_hour = time_info.hour,
        .tm_mday = time_info.day,
        .tm_mon = time_info.month - 1,
        .tm_year = time_info.year + 100
    };
    time_t t = mktime(&esp_time_info);
    timeval now = {
        .tv_sec = t
    };
    settimeofday(&now, NULL);
}


// Function to get the ESP32 internal RTC time as a tm struct
struct tm getInternalRTC() {
    struct timeval now;
    gettimeofday(&now, NULL);  // Get ESP32 internal RTC time

    struct tm* current_time = localtime(&now.tv_sec);  // Convert to human-readable format
    return *current_time; // Return time as a struct
}


// Function to print the internal RTC time
void printInternalRTC() {
    struct tm current_time = getInternalRTC();

    printf("ESP32 Internal RTC: %04d-%02d-%02d %02d:%02d:%02d\n",
           current_time.tm_year + 1900, current_time.tm_mon + 1,
           current_time.tm_mday, current_time.tm_hour,
           current_time.tm_min, current_time.tm_sec);
}


time_t getEpochTime() {
    struct timeval now;
    gettimeofday(&now, NULL);  // Retrieves the synced ESP32 RTC time
    return now.tv_sec;         // Returns UNIX timestamp (epoch)
}


String getFormattedTime() {
  struct timeval now;
  gettimeofday(&now, NULL); // Read ESP32 internal RTC time

  struct tm* timeinfo = localtime(&now.tv_sec); // Convert to broken-down time

  char buffer[30];
  strftime(buffer, sizeof(buffer), "%d/%m/%Y %I:%M %p", timeinfo);
  return String(buffer);  // Return formatted string
}


