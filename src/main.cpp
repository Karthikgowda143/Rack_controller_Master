#include <Arduino.h>

void SensorManagementTask(void *pvParameters);
void ElectronAppCommunicationTask(void *pvParameters);
void SlaveCommunicationTask(void *pvParameters);

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32-S3 USB Serial port started");

  // Create Sensor Management Thread
  xTaskCreatePinnedToCore(
    SensorManagementTask,    // Task function
    "Sensor Task",           // Name
    4096,                    // Stack size
    NULL,                    // Parameters
    1,                       // Priority
    NULL,                    // Task handle
    1                        // Run on Core 1
  );

  // Create Electron App Communication Thread
  xTaskCreatePinnedToCore(
    ElectronAppCommunicationTask,
    "Electron Task",
    4096,
    NULL,
    1,
    NULL,
    1
  );

  // Create Slave Communication Thread
  xTaskCreatePinnedToCore(
    SlaveCommunicationTask,
    "Slave Task",
    4096,
    NULL,
    1,
    NULL,
    0 // Run on Core 0 (to balance load)
  );

}

void loop() {
  // Main loop can be empty as tasks are running in FreeRTOS

}
