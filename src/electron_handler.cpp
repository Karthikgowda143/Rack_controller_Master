
#include "electron_handler.h"

unsigned long previousMillis = 0;

void ElectronAppCommunicationTask(void *pvParameters) {

  vTaskDelay(pdMS_TO_TICKS(5000));
  Serial0.begin(115200, SERIAL_8N1, 42, 43); // for 4G communication
  Serial2.begin(9600, SERIAL_8N1, 18, 17); // RX = 18, TX = 17

  while (1) {

    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= 5000) {
      previousMillis = currentMillis;
      
      sendDataToElectron();
    }

   // unsigned long currentMillis = millis();
  static unsigned long lastLogTime = 0;
  if (currentMillis - lastLogTime >= LOG_INTERVAL) {
    lastLogTime = currentMillis;
        logData("PERIODIC", "-", "Routine status log"); // Log periodic data every LOG_INTERVAL milliseconds
  }



    if(Serial2.available()) {
      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, Serial2);
      if(!error) {
        const char* handshake = doc["handshake"];
        if(handshake && strcmp(handshake, "ping") == 0) {
          Serial.println("Handshake ping received from Electron App.");
          StaticJsonDocument<64> responseDoc;
          responseDoc["handshake"] = "pong";
          serializeJson(responseDoc, Serial2);
          Serial2.println();
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}


void sendDataToElectron()
{
  if(virginmodeflag == false){
  
      StaticJsonDocument<512> doc;
      doc["type"] = "sensors_data";
      doc["rack"] = rackID; 
      doc["temp"] = temperature;
      doc["humidity"] = humidity;
      doc["smoke"] = smokeValue;
      doc["batt"] = batteryVoltage;

      for(int i = 0; i < 4; i++) {
        if(doorarray[i] == 1) doc["door" + String(i + 1)] = "Open";
        else doc["door" + String(i + 1)] = "Closed";
      }

      for(int i = 0; i < 4; i++) {
        if(normal_fanarray[i] == 1) doc["fan" + String(i + 1)] = "ON";
        else doc["fan" + String(i + 1)] = "OFF";
      }

      for(int i = 0; i < 4; i++) {
        if(standby_fanarray[i] == 1) doc["fan" + String(i + 5)] = "ON";
        else doc["fan" + String(i + 5)] = "OFF";
      }

      // serializeJson(doc, Serial2);
      // Serial2.println();

    // Serialize to buffer
    char jsonBuffer[512];
    size_t len = serializeJson(doc, jsonBuffer, sizeof(jsonBuffer));
    jsonBuffer[len] = '\n'; // Add newline for easier parsing

    // Send to Electron
    Serial2.write((uint8_t*)jsonBuffer, len + 1);

    if(four_g_enable == 1){
    // Send to 4G 
      Serial.println("4G communication started...");
      Serial0.write((uint8_t*)jsonBuffer, len + 1);
    }
  }
}