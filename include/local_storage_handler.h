#ifndef LOCAL_STORAGE_HANDLER_H
#define LOCAL_STORAGE_HANDLER_H

#include "main.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "sensor_handler.h"

// -------------------- CONFIG --------------------     
#define LOG_FOLDER    "/Rack_" + String(rackID) + "_logs"
#define MAX_LOG_DAYS  30
#define LOG_INTERVAL  60000    // 5 min for periodic logs (example)

#define SD_MISO   13
#define SD_MOSI   11
#define SD_SCK    12
#define SD_CS     10


void checkSDCard();
void setupLocalStorage();
void writeLog(String entry);
//void logData(String logType, String eventType, String description);
time_t dateStringToTime(const char* dateStr);
void deleteOldLogs();


// Adjust as needed
#define LOG_QUEUE_LENGTH 30
#define LOG_ENTRY_MAXLEN 256

//extern bool sdMounted;
//extern const char* LOG_FOLDER;

void initLogger();   // Call in setup()
void logData(String logType, String eventType, String description);




#endif // LOCAL_STORAGE_HANDLER_H