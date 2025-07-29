
#include "spiffs_handler.h"

#define CONFIG_FILE    "/config.json"
#define RACK_FILE      "/rackid.json"
#define PASSWORD_FILE  "/password.json"
#define SLAVE_VIRGIN "/slave_virgin.json"
#define BUTTON_STATUS "/button_status.json"

void saveConfigSettings(int tempThreshold, int smokeThreshold, int voltageRange, int humidity_enable, int four_g_enable) {
  DynamicJsonDocument doc(256);
  doc["tempThreshold"] = tempThreshold;
  doc["smokeThreshold"] = smokeThreshold;
  doc["voltageRange"] = voltageRange;
  doc["humidity_enable"] = humidity_enable;
  doc["four_g_enable"] = four_g_enable;

  File file = SPIFFS.open(CONFIG_FILE, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open config file for writing");
    return;
  }

  serializeJsonPretty(doc, file);
  file.close();
  Serial.println("Saved config settings");
}

void saveRackID(int value) {
  DynamicJsonDocument doc(32);
  doc["rackID"] = value;

  File file = SPIFFS.open(RACK_FILE, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open rackid file");
    return;
  }

  serializeJson(doc, file);
  file.close();

  rackID = value;
  Serial.printf("Saved rack ID: %d\n", value);
}

void savePassword(int value) {
  DynamicJsonDocument doc(32);
  doc["password"] = value;

  File file = SPIFFS.open(PASSWORD_FILE, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open password file");
    return;
  }

  serializeJson(doc, file);
  file.close();

  password = value;
  Serial.printf("Saved password: %d\n", value);
}

void saveSlave_virgin(bool virgin) {
    DynamicJsonDocument doc(64);
    doc["slave_virgin"] = virgin;

    File file = SPIFFS.open(SLAVE_VIRGIN, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open slave virgin file");
        return;
    }
    serializeJson(doc, file);
    file.close();
}

void saveButtonStatus( bool startStopButtonPressed) {
  DynamicJsonDocument doc(64);
  doc["startStopButtonPressed"] = startStopButtonPressed;

  File file = SPIFFS.open(BUTTON_STATUS, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open button status file");
    return;
  }
  serializeJson(doc, file);
  file.close();
}
   

void loadAllSettings() {
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed.");
    return;
  }

//saveConfigSettings(31,650,2,1,1);
//saveSlave_virgin(false);

  // Load config.json
  File file = SPIFFS.open(CONFIG_FILE, FILE_READ);
  if (file) {
    DynamicJsonDocument doc(256);
    if (deserializeJson(doc, file) == DeserializationError::Ok) {
      tempThreshold   = doc["tempThreshold"];
      smokeThreshold  = doc["smokeThreshold"];
      voltageRange    = doc["voltageRange"];
      humidity_enable = doc["humidity_enable"] | 0;
      four_g_enable   = doc["four_g_enable"] | 0;
    }
    file.close();
  }

  // Load rackid.json
  file = SPIFFS.open(RACK_FILE, FILE_READ);
  if (file) {
    DynamicJsonDocument doc(64);
    if (deserializeJson(doc, file) == DeserializationError::Ok) {
      rackID = doc["rackID"];
    }
    file.close();
  }

  // Load password.json
  file = SPIFFS.open(PASSWORD_FILE, FILE_READ);
  if (file) {
    DynamicJsonDocument doc(64);
    if (deserializeJson(doc, file) == DeserializationError::Ok) {
      password = doc["password"];
    }
    file.close();
  }

  bool slave_virgin = true;
  //Load virgin.json
  file = SPIFFS.open(SLAVE_VIRGIN, FILE_READ);
  if (file) {
    DynamicJsonDocument doc(64);
    if (deserializeJson(doc, file) == DeserializationError::Ok) {
        slave_virgin = doc["virgin"];
    }
    file.close();
  }

  file = SPIFFS.open(BUTTON_STATUS, FILE_READ);
  if (file) {
    DynamicJsonDocument doc(64);
    if (deserializeJson(doc, file) == DeserializationError::Ok) {
      startStopButtonPressed = doc["startStopButtonPressed"];
    }
    file.close();
  }

  // Debug
  Serial.println("Loaded all settings:");
  Serial.printf("tempThreshold   : %d\n", tempThreshold);
  Serial.printf("smokeThreshold  : %d\n", smokeThreshold);
  Serial.printf("voltageRange    : %d\n", voltageRange);
  Serial.printf("humidity_enable : %d\n", humidity_enable);
  Serial.printf("four_g_enable   : %d\n", four_g_enable);
  Serial.printf("rackID          : %d\n", rackID);
  Serial.printf("password        : %d\n", password);
  Serial.printf("Slave virgin    : %d\n", slave_virgin);
  Serial.printf("startStopButtonPressed : %d\n", startStopButtonPressed);


  if(tempThreshold == 0 || smokeThreshold == 0 || slave_virgin == true){
    Serial.println("Settings are not set. Please set them in the web interface.");
    virginmodeflag = true;
  }
  else{
    virginmodeflag = false;
    //normalFanOn();
  }

  if(startStopButtonPressed == true){
    normalFanOn();
  }else{
    normalFanOff();
  }
}


