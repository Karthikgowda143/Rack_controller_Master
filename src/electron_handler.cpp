
#include "electron_handler.h"
#define SERIAL_ELECTRON Serial2

unsigned long previousMillis = 0;

void ElectronAppCommunicationTask(void *pvParameters) {

  vTaskDelay(pdMS_TO_TICKS(5000));
  Serial0.begin(115200, SERIAL_8N1, 42, 43); // for 4G communication
  SERIAL_ELECTRON.begin(9600, SERIAL_8N1, 18, 17); // RX = 18, TX = 17

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
        //checkSDCard();
        //logData("PERIODIC", "-", "Routine status log"); // Log periodic data every LOG_INTERVAL milliseconds
  }



  //readElectronData(); // Read data from Electron App
    // if(Serial2.available()) {
    //   StaticJsonDocument<512> doc;
    //   DeserializationError error = deserializeJson(doc, Serial2);
    //   if(!error) {
    //     const char* handshake = doc["handshake"];
    //     if(handshake && strcmp(handshake, "ping") == 0) {
    //       Serial.println("Handshake ping received from Electron App.");
    //       StaticJsonDocument<64> responseDoc;
    //       responseDoc["handshake"] = "pong";
    //       serializeJson(responseDoc, Serial2);
    //       Serial2.println();
    //     }
    //   }
    // }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

char electronBuffer[512];

void readElectronData() {
  if(SERIAL_ELECTRON.available()) {
    memset(electronBuffer, 0, sizeof(electronBuffer)); // Clear buffer
    size_t len = SERIAL_ELECTRON.readBytesUntil('\n', electronBuffer, sizeof(electronBuffer) - 1);
    electronBuffer[len] = '\0'; // Null terminate the string

    Serial.printf("Received from Electron: %s\n", electronBuffer);

    // Parse JSON
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, electronBuffer);
    if (error) {
      Serial.println("JSON Parse Error!");
      return;
    }

    // Process the message
    const char* type = doc["type"];
    if (strcmp(type, "handshake") == 0) {
      Serial.println("Handshake received from Electron App.");
      StaticJsonDocument<64> responseDoc;
      responseDoc["handshake"] = "pong";
      serializeJson(responseDoc, SERIAL_ELECTRON);
      SERIAL_ELECTRON.println();
    }
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

     

    // Serialize to buffer
    char jsonBuffer[512];
    size_t len = serializeJson(doc, jsonBuffer, sizeof(jsonBuffer));
    jsonBuffer[len] = '\n'; // Add newline for easier parsing

    // Send to Electron
    SERIAL_ELECTRON.write((uint8_t*)jsonBuffer, len + 1);

    if(four_g_enable == 1){
    // Send to 4G 
      Serial.println("4G communication started...");
      Serial0.write((uint8_t*)jsonBuffer, len + 1);
    }
  }
}