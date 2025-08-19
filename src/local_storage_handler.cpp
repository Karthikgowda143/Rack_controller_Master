#include "local_storage_handler.h"
#include "sensor_handler.h"


// void setupLocalStorage() {
//     // Initialize SD card
//     SD.end();
//     SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
//     Serial.println("Initializing SD card...");
//     if (!SD.begin(SD_CS)) {
//         Serial.println("SD card initialization failed!");
//         sdMounted = false;
//         return;
//     }
//     Serial.println("SD card initialized successfully.");

//     sdMounted = true;
//     // Create log folder if it doesn't exist
//     if (!SD.exists(LOG_FOLDER)) {
//         SD.mkdir(LOG_FOLDER);
//         Serial.println("Log folder created.");
//     } else {
//         Serial.println("Log folder already exists.");
//     }

//     // Delete files older than 30 days in /logs
//     deleteOldLogs();
// }

void setupLocalStorage() {
    delay(500);  // let SD card power stabilize
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

    for (int i = 0; i < 5; i++) {
        if (SD.begin(SD_CS)) {
            Serial.println("SD card initialized.");
            sdMounted = true;
            break;
        }
        Serial.println("Retrying SD init...");
        delay(500);
    }

    if (!sdMounted) {
        Serial.println("SD card initialization failed!");
        sdMounted = false;
        return;
    }

    if (!SD.exists(LOG_FOLDER)) {
        SD.mkdir(LOG_FOLDER);
    }
    //deleteOldLogs();
}


// Check if card is still available
void checkSDCard() {
    Serial.println("Checking SD Card...");
    if (sdMounted) {
        // Test with cardType()
        if (SD.cardType() == CARD_NONE) {
            Serial.println(" SD Card removed!");
            sdMounted = false;
        }
    } else {
        setupLocalStorage();
        // Try to re-mount if card was re-inserted
        if (SD.cardType() != CARD_NONE) {
            Serial.println("Re-mounting SD Card...");
            setupLocalStorage();
        }
    }
}

// // Write log entry to today's file with headers if needed
// void writeLog(String entry) {
//   String dateStr = getFormattedDate(); // Get current date in dd/mm/yyyy format
//   String filename = String(LOG_FOLDER) + "/" + dateStr + ".csv";

//   Serial.println("Writing log to: " + filename);
//   // Ensure the log folder exists (rackID may be set after setupLocalStorage)
//   if (!SD.exists(LOG_FOLDER)) {
//       SD.mkdir(LOG_FOLDER);
//   }

//   bool newFile = !SD.exists(filename);

//   File file;
//   if (newFile) {
//     // Create the file in write mode if it doesn't exist
//     file = SD.open(filename, FILE_WRITE);
//   } else {
//     // Append when the file already exists
//     file = SD.open(filename, FILE_APPEND);
//   }
//   if (!file) {
//     Serial.println("Failed to open log file for appending");
//     Serial.printf("Path: %s\n", filename.c_str());
//     sdMounted = false;
//     return;
//   }

//   // Write column headers if file is new
//   if (newFile) {
//     file.println("Time,Log Type,Event Type,Volt(VDC),Temp(°C),Humidity(%),Smoke(PPM),"
//                  "Normal Fans, Standby Fans, Front Door,Back Door,Side Door1,Side Door2,Description");
//   }

//   // Append the log entry
//   file.println(entry);
//   file.close();

//   Serial.println("Logged: " + entry);
// }

// void logData(String logType, String eventType, String description) {
//     // Build CSV entry using String array for easy joining
//     String fields[] = {
//         getFormattedTime(),
//         logType,
//         eventType,
//         String(batteryVoltage),
//         String(temperature),
//         String(humidity),
//         String(smokeValue),
//        (normal_fanarray[1] ==1 )  ? "ON" : "OFF", 
//        (standby_fanarray[1] ==1 )  ? "ON" : "OFF",
//        (doorarray[0] ==1 )  ? "Open"   : "Closed",
//        (doorarray[1] ==1 )  ? "Open"   : "Closed",
//        (doorarray[2] ==1 )  ? "Open"   : "Closed",
//        (doorarray[3] ==1 )  ? "Open"   : "Closed",
//         description 
//     };

//     // Join fields with commas
//     String entry;
//     for (size_t i = 0; i < sizeof(fields)/sizeof(fields[0]); i++) {
//         entry += fields[i];
//         if (i < sizeof(fields)/sizeof(fields[0]) - 1) entry += ",";
//     }

//     writeLog(entry);
// }

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

//--------------------------------

// #include "logger_handler.h"
// #include "sensor_handler.h"   // for batteryVoltage, temp, humidity, etc.
// #include "local_storage_handler.h"  // for getFormattedDate(), getFormattedTime(), deleteOldLogs()

QueueHandle_t logQueue;
//bool sdMounted = false;
//const char* LOG_FOLDER = "/logs";   // your folder, or /Rack_01_logs

// Build one CSV log entry (same as before)
String buildCSVLine(String logType, String eventType, String description) {
    String fields[] = {
        getFormattedTime(),
        logType,
        eventType,
        String(batteryVoltage),
        String(temperature),
        String(humidity),
        String(smokeValue),
        (normal_fanarray[1] == 1) ? "ON" : "OFF",
        (standby_fanarray[1] == 1) ? "ON" : "OFF",
        (doorarray[0] == 1) ? "Open" : "Closed",
        (doorarray[1] == 1) ? "Open" : "Closed",
        (doorarray[2] == 1) ? "Open" : "Closed",
        (doorarray[3] == 1) ? "Open" : "Closed",
        description
    };

    String entry;
    for (size_t i = 0; i < sizeof(fields) / sizeof(fields[0]); i++) {
        entry += fields[i];
        if (i < sizeof(fields) / sizeof(fields[0]) - 1) entry += ",";
    }
    return entry;
}

// Writes entry to SD (runs only inside LoggerTask)
void writeLog(String entry) {
    if (!sdMounted) {
        Serial.println("SD not mounted, skipping log");
        return;
    }

    String dateStr = getFormattedDate(); // DD-MM-YYYY
    String filename = String(LOG_FOLDER) + "/" + dateStr + ".csv";

    bool newFile = !SD.exists(filename);

    File file = SD.open(filename, newFile ? FILE_WRITE : FILE_APPEND);
    if (!file) {
        Serial.println("Failed to open log file");
        sdMounted = false;
        return;
    }

    if (newFile) {
        file.println("Time,Log Type,Event Type,Volt(VDC),Temp(°C),Humidity(%),Smoke(PPM),"
                     "Normal Fans,Standby Fans,Front Door,Back Door,Side Door1,Side Door2,Description");

                     deleteOldLogs();
    }

    file.println(entry);
    file.close();
    Serial.println("Logged: " + entry);
}

// Logger task: waits for messages and writes to SD
void loggerTask(void* pvParams) {
    char entryBuf[LOG_ENTRY_MAXLEN];

    while (true) {
        if (xQueueReceive(logQueue, &entryBuf, portMAX_DELAY) == pdTRUE) {
            writeLog(String(entryBuf));
        }
    }
}

// Init queue + task
void initLogger() {
    logQueue = xQueueCreate(LOG_QUEUE_LENGTH, LOG_ENTRY_MAXLEN);
    if (logQueue == NULL) {
        Serial.println("Failed to create log queue!");
        return;
    }

    xTaskCreatePinnedToCore(loggerTask, "LoggerTask", 4096, NULL, 1, NULL, 1);
    Serial.println("Logger task started");
}

// Called by other tasks
void logData(String logType, String eventType, String description) {
    String entry = buildCSVLine(logType, eventType, description);

    if (logQueue != NULL) {
        char buf[LOG_ENTRY_MAXLEN];
        strncpy(buf, entry.c_str(), LOG_ENTRY_MAXLEN - 1);
        buf[LOG_ENTRY_MAXLEN - 1] = '\0';
        if (xQueueSend(logQueue, &buf, 0) != pdTRUE) {
            Serial.println("Log queue full, dropping entry!");
        }
    }
}
