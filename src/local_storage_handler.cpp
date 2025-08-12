#include "local_storage_handler.h"
#include "sensor_handler.h"


void setupLocalStorage() {
    // Initialize SD card
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    Serial.println("Initializing SD card...");
    if (!SD.begin(SD_CS)) {
        Serial.println("SD card initialization failed!");
        return;
    }
    Serial.println("SD card initialized successfully.");

    // Create log folder if it doesn't exist
    if (!SD.exists(LOG_FOLDER)) {
        SD.mkdir(LOG_FOLDER);
        Serial.println("Log folder created.");
    } else {
        Serial.println("Log folder already exists.");
    }

    // Delete files older than 30 days in /logs
    deleteOldLogs();
}

// Write log entry to today's file with headers if needed
void writeLog(String entry) {
  String dateStr = getFormattedDate(); // Get current date in dd/mm/yyyy format
  String filename = String(LOG_FOLDER) + "/" + dateStr + ".csv";

  bool newFile = !SD.exists(filename);

  File file = SD.open(filename, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open log file for appending");
    return;
  }

  // Write column headers if file is new
  if (newFile) {
    file.println("Time,Rack ID,Log Type,Event Type,Volt (V),Temp (C),Humidity(%),Smoke (PPM),"
                 "Fan1,Fan2,Fan3,Fan4,Fan5,Fan6,Fan7,Fan8,"
                 "Door1,Door2,Door3,Door4,Description");
  }

  // Append the log entry
  file.println(entry);
  file.close();

  Serial.println("Logged: " + entry);
}

void logData(String logType, String eventType, String description) {
    // Arrays for fan and door states
    const char* nor_fan[4];
    const char* stand_fan[4];
    const char* door[4];

    // Populate state arrays
    for (int i = 0; i < 4; i++) {
        nor_fan[i]   = (normal_fanarray[i] ==1 )  ? "ON"     : "OFF";
        stand_fan[i] = (standby_fanarray[i] == 1) ? "ON"     : "OFF";
        door[i]      = (doorarray[i] == 1)        ? "Open"   : "Closed";
    }

    // Build CSV entry using String array for easy joining
    String fields[] = {
        getFormattedTime(),
        String(rackID),
        logType,
        eventType,
        String(batteryVoltage),
        String(temperature),
        String(humidity),
        String(smokeValue),
        nor_fan[0], nor_fan[1], nor_fan[2], nor_fan[3],
        stand_fan[0], stand_fan[1], stand_fan[2], stand_fan[3],
        door[0], door[1], door[2], door[3],
        description 
    };

    // Join fields with commas
    String entry;
    for (size_t i = 0; i < sizeof(fields)/sizeof(fields[0]); i++) {
        entry += fields[i];
        if (i < sizeof(fields)/sizeof(fields[0]) - 1) entry += ",";
    }

    writeLog(entry);
}

// Convert "YYYY-MM-DD" → time_t
time_t dateStringToTime(const char* dateStr) {
    struct tm t = {};
    sscanf(dateStr, "%4d-%2d-%2d", &t.tm_year, &t.tm_mon, &t.tm_mday);
    t.tm_year -= 1900;  // struct tm years since 1900
    t.tm_mon  -= 1;     // struct tm months are 0-11
    return mktime(&t);
}

// Delete log files older than MAX_LOG_DAYS
void deleteOldLogs() {
    File dir = SD.open(LOG_FOLDER);
    if (!dir) {
        Serial.println("Failed to open log folder");
        return;
    }

    time_t now;
    struct timeval tv;
    gettimeofday(&tv, NULL); 
    now = tv.tv_sec;

    while (true) {
        File file = dir.openNextFile();
        if (!file) break; // No more files

        String fileName = file.name();
        file.close();
        Serial.printf("Checking file: %s\n", fileName.c_str());
        // Expecting format: DD-MM-YYYY.csv
        if (fileName.length() >= 14 && fileName.endsWith(".csv")) {
            String datePart = fileName.substring(0, 10); // Extract DD-MM-YYYY
            // Convert DD-MM-YYYY to YYYY-MM-DD for dateStringToTime
            String day   = datePart.substring(0, 2);
            String month = datePart.substring(3, 5);
            String year  = datePart.substring(6, 10);
            String isoDate = year + "-" + month + "-" + day;
            time_t fileTime = dateStringToTime(isoDate.c_str());

            Serial.printf("File date: %s, time_t: %ld\n", isoDate.c_str(), fileTime);
            double daysOld = difftime(now, fileTime) / (60 * 60 * 24);
            Serial.printf("Days old: %.0f\n", daysOld);
            if (daysOld > MAX_LOG_DAYS) {
                String fullPath = String(LOG_FOLDER) + "/" + fileName;
                if (SD.remove(fullPath)) {
                    Serial.printf("Deleted old log: %s (%.0f days old)\n", fullPath.c_str(), daysOld);
                } else {
                    Serial.printf("Failed to delete: %s\n", fullPath.c_str());
                }
            }
        }
    }
    dir.close();
}