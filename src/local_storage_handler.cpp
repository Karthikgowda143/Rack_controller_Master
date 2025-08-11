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
    file.println("Timestamp,Rack ID,Log Type,Event Type,Volt (V),Temp (C),Humidity(%),Smoke (PPM),"
                 "Fan1,Fan2,Fan3,Fan4,Fan5,Fan6,Fan7,Fan8,"
                 "Door1,Door2,Door3,Door4,Description");
  }

  // Append the log entry
  file.println(entry);
  file.close();

  Serial.println("Logged: " + entry);
}

// void logPeriodicData() {
//   String nor_fan[4];
//   String stand_fan[4];
//   String door[4];
//   for(int i=0; i < 4; i++) {
//     (normal_fanarray[i] == 1) ? nor_fan[i] = "ON" : nor_fan[i] = "OFF";
//     (standby_fanarray[i] == 1) ? stand_fan[i] = "ON" : stand_fan[i] = "OFF";
//     (doorarray[i] == 1) ? door[i] = "Open" : door[i] = "Closed";
//   }
//     String entry = getFormattedDate(); + "," + String(rackID) + "," + "PERIODIC" + "," + "-" + "," +
//                    String(batteryVoltage) + "," + String(temperature) + "," + String(humidity) + "," + String(smokeValue) + "," +
//                    nor_fan[0]+ "," + nor_fan[1] + "," + nor_fan[2] + "," + nor_fan[3] + "," +
//                    stand_fan[0] + "," + stand_fan[1] + "," + stand_fan[2] + "," + stand_fan[3] + "," +
//                    door[0] + "," + door[1] + "," + door[2] + "," + door[3] + "," +
//                    "Periodic Data";
//     writeLog(entry);
// }

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