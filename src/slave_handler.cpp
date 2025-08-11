#include "slave_handler.h"
#include "slip.c"

//#include "clock.h"


uint8_t buf[500];

unsigned long previousMillis2 = 0;

void SlaveCommunicationTask(void *pvParameters) {

Serial1.begin(9600, SERIAL_8N1, 15, 16);
Serial.println("UART1 initialized.");
slip_init_handler();

  while (1) {
    processSLIP();  

    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis2 >= 5000) {  // Check every 5 seconds
        previousMillis2 = currentMillis;

        if(virginmodeflag == false) {
            sendInternalDisplayData();
            checkDoorOpen();
        } else {
            send_virgin_message();
        }
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// Function to process incoming SLIP messages
void processSLIP() {
    while (Serial1.available()) {
        uint8_t receivedByte = Serial1.read();
        slip_process_byte(receivedByte);
    }
}

void recv_message(uint8_t *data, uint32_t size)
{
    char jsonBuffer[size + 1];
    memcpy(jsonBuffer, data, size);
    jsonBuffer[size] = '\0';  // Null terminate the string

    Serial.printf("Received JSON: %s\n", jsonBuffer);
    // Parse JSON
    //StaticJsonDocument<200> doc;
    DynamicJsonDocument doc(512);  
    DeserializationError error = deserializeJson(doc, jsonBuffer);

    if (error) {
        Serial.println("JSON Parse Error!");
        return;
    }

     // Check Message Type
     String type = doc["type"].as<String>();
    //const char* type = doc["type"];

    if (type == "door_status") {
        doorarray[0] = doc["front"];
        doorarray[1] = doc["back"];
        doorarray[2] = doc["side1"];
        doorarray[3] = doc["side2"];

        if(doorarray[0] == 1 || doorarray[1] == 1)
        indicators[2] = 1;
        else 
        indicators[2] = 0;
    }
    else if(type == "virgin_mode") {
       bool virginmode = doc["virgin_mode"].as<bool>();
       saveSlave_virgin(virginmode);
       loadAllSettings();
    }
    else if (type == "start_stop_button_pressed"){
        startStopButtonPressed = doc["status"].as<bool>();
        saveButtonStatus(startStopButtonPressed);
        send_button_status(startStopButtonPressed);
        if(startStopButtonPressed == true){
            normalFanOn();
        } else{
            normalFanOff();
        }
    }
    else if (type == "button_status_request") {
        Serial.println ("Recieved Button Status Request");
        send_button_status(startStopButtonPressed);
    }
    else if (type == "changePassword") {
        int oldPassword = doc["oldPassword"];
        int newPassword = doc["newPassword"];
        if( compareWithStoredPassword(oldPassword)) {
            password = newPassword;
            savePassword(password);
        }
    }
    else if (type == "password") {
        int enteredPassword = doc["password"];
        if (compareWithStoredPassword(enteredPassword)) {
            sendDoorAndLightOpen();
        }
    }
    else if (type == "scan") {
        const char* userID = doc["userID"];
        
        sendDoorAndLightOpen(); 
    }
    else if (type == "rack_id_set") {
        int rackIDReceived = doc["rack_id"];
        saveRackID(rackIDReceived);
        loadAllSettings();
    }
    else if (type == "device_settings_set") {

        tempThreshold = doc["temperature_threshold"];
        smokeThreshold = doc["smoke_threshold"];
        voltageRange = doc["voltage_range"];
        humidity_enable = doc["humidity_enable"];
        four_g_enable = doc["four_g_enable"];

        
        saveConfigSettings(tempThreshold, smokeThreshold, voltageRange, humidity_enable, four_g_enable);
        delay(50);
        loadAllSettings();
        set_voltage_range();  
        if(four_g_enable == 1){
            StaticJsonDocument<256> doc;
            doc["type"] = "4g_status";
            doc["status"] = "ON";
            serializeJson(doc, Serial0);
            Serial0.println();
        }
        else{
            StaticJsonDocument<256> doc;
            doc["type"] = "4g_status";
            doc["status"] = "OFF";
            serializeJson(doc, Serial0);
            Serial0.println();
        }
        
    }
    else if (type == "rtc_set") {
        int year = doc["year"];
        int month = doc["month"];
        int day = doc["day"];
        int hour = doc["hour"];
        int minute = doc["minute"];
        int meridiem = doc["meridiem"];

        // Sync RTC with received time
        rtc_SetTime(year-2000, month, day, hour + (meridiem == 1 ? 12 : 0), minute, 0);
    }
    else if(type == "device_settings_request")
    {
        send_device_settings();
    }
}

#define DOOR_COUNT 4
#define ALERT_TIMEOUT 300000      // 5 minutes
#define ALARM_INTERVAL 5000       // Alarm repeat every 5 sec

unsigned long door_open_time[DOOR_COUNT] = {0};
unsigned long last_alarm_time[DOOR_COUNT] = {0};
bool is_door_open[DOOR_COUNT] = {false};
bool alert_sent[DOOR_COUNT] = {false};

void checkDoorOpen() {
    const char* doorNames[DOOR_COUNT] = { "Front", "Back", "Side1", "Side2" };
    unsigned long now = millis();

    for (int i = 0; i < DOOR_COUNT; i++) {
        bool doorOpen = (doorarray[i] == 1);

        if (doorOpen) {
            if (!is_door_open[i]) {
                is_door_open[i] = true;
                door_open_time[i] = now;
                alert_sent[i] = false;
                last_alarm_time[i] = 0;
            }

            // If door open > timeout
            if ((now - door_open_time[i]) >= ALERT_TIMEOUT) {

                //  Repeat alarm every 5 sec
                if ((now - last_alarm_time[i]) >= ALARM_INTERVAL) {
                    send_door_emergency(i);
                    last_alarm_time[i] = now;
                }

                //Send alert/log only once
                if (!alert_sent[i]) {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "%s Door open timeout", doorNames[i]);
                    //send_sms_alert(doorNames[i]);
                    //send_email_alert(doorNames[i]);
                    logData("EVENT", "DOOR TIMEOUT", buf);
            
                    alert_sent[i] = true;
                }
            }

        } else {
            // Door is closed: reset all flags/timers
            is_door_open[i] = false;
            door_open_time[i] = 0;
            last_alarm_time[i] = 0;
            alert_sent[i] = false;
        }
    }
}

void send_door_emergency( int door) {
    StaticJsonDocument<253> doc;
    doc["type"] = "door_emergency";

    char jsonBuffer[256];
    serializeJson(doc, jsonBuffer, sizeof(jsonBuffer)); 
    slip_send_message( &slip, (uint8_t *)jsonBuffer, strlen(jsonBuffer)); 
}

void send_restart_message() {
    StaticJsonDocument<253> doc;
    doc["type"] = "slave_restart";

    char jsonBuffer[256];
    serializeJson(doc, jsonBuffer, sizeof(jsonBuffer)); 
    slip_send_message( &slip, (uint8_t *)jsonBuffer, strlen(jsonBuffer)); 
}

void send_button_status(bool status) {
    StaticJsonDocument<256> doc;
    doc["type"] = "start_stop_button_pressed";
    doc["status"] = status;

    char jsonBuffer[256];
    serializeJson(doc, jsonBuffer, sizeof(jsonBuffer)); 
    slip_send_message( &slip, (uint8_t *)jsonBuffer, strlen(jsonBuffer)); 
}

void send_device_settings() {
    StaticJsonDocument<256> doc;
    doc["type"] = "device_settings";
    doc["rack_id"] = rackID;
    doc["temperature_threshold"] = tempThreshold;
    doc["smoke_threshold"] = smokeThreshold;
    doc["voltage_range"] = voltageRange;
    doc["humidity_enable"] = humidity_enable;
    doc["four_g_enable"] = four_g_enable;

    char jsonBuffer[256];
    serializeJson(doc, jsonBuffer);
    slip_send_message(&slip, (uint8_t *)jsonBuffer, strlen(jsonBuffer));
}

void send_virgin_message() {
    StaticJsonDocument<256> doc;
    doc["type"] = "virgin_mode";
    doc["status"] = virginmodeflag;

    char jsonBuffer[256];
    serializeJson(doc, jsonBuffer);
    slip_send_message(&slip, (uint8_t *)jsonBuffer, strlen(jsonBuffer));
}

void sendInternalDisplayData() {

    DynamicJsonDocument doc(512);

    doc["type"] = "display";
   // doc["rack_id"] = rackID;  // Add rack ID to JSON
    doc["temperature"] = temperature;

    if(humidity_enable == 1) {
        doc["humidity"] = humidity;  
    } else {
        doc["humidity"] = 0;  
    }

    doc["smoke"] = smokeValue;
    doc["voltage"] = batteryVoltage;
    //doc["four_g_enable"] = four_g_enable;
    
    JsonArray normalfanarray = doc.createNestedArray("normal_fanarray");
    for (int i = 0; i < 4; i++) normalfanarray.add(normal_fanarray[i]);

    JsonArray standbyfanarray = doc.createNestedArray("standby_fanarray");
    for (int i = 0; i < 4; i++) standbyfanarray.add(standby_fanarray[i]);

    JsonArray doorStatus = doc.createNestedArray("doorarray");
    for (int i = 0; i < 4; i++) doorStatus.add(doorarray[i]);

    JsonArray indicatorstatus = doc.createNestedArray("indicatorarray");
    for (int i = 0; i < 8; i++) indicatorstatus.add(indicators[i]);

    delay(10);
    currentEpochTime = getEpochTime();  // Get current epoch time
    
    doc["epoch_time"] = currentEpochTime;  // Add epoch time to JSON

    char jsonBuffer[256];
    serializeJson(doc, jsonBuffer);
    
    slip_send_message(&slip, (uint8_t *)jsonBuffer, strlen(jsonBuffer));
}

void sendLightOn() {
    DynamicJsonDocument doc(128);
    doc["type"] = "light_on";

    char jsonBuffer[128];
    serializeJson(doc, jsonBuffer);

    slip_send_message(&slip, (uint8_t *)jsonBuffer, strlen(jsonBuffer));
}

void sendLightOff() {
    DynamicJsonDocument doc(128);
    doc["type"] = "light_off";

    char jsonBuffer[128];
    serializeJson(doc, jsonBuffer);

    slip_send_message(&slip, (uint8_t *)jsonBuffer, strlen(jsonBuffer));
}

void sendchangepasswordACK(int status) {
    DynamicJsonDocument doc(128);
    doc["type"] = "changePasswordACK";
    doc["status"] = status;

    char jsonBuffer[128];
    serializeJson(doc, jsonBuffer);

    slip_send_message(&slip, (uint8_t *)jsonBuffer, strlen(jsonBuffer));
}


void sendACK(const char* msgType, int status) {
    DynamicJsonDocument doc(128);
    doc["type"] = "ack";
    doc["msg_type"] = msgType;
    doc["status"] = status;

    char jsonBuffer[128];
    serializeJson(doc, jsonBuffer);

    slip_send_message(&slip, (uint8_t *)jsonBuffer, strlen(jsonBuffer));
}


bool compareWithStoredPassword(int enteredPassword) {
    return enteredPassword == password;
}

void sendDoorAndLightOpen() {
    DynamicJsonDocument doc(128);
    doc["type"] = "door_light_open";

    char jsonBuffer[128];
    serializeJson(doc, jsonBuffer);

    slip_send_message(&slip, (uint8_t *)jsonBuffer, strlen(jsonBuffer));
}

uint8_t write_byte(uint8_t byte) 
{
        //printf("TX: %02X\n", byte);

        Serial1.write(byte);
        return 1;
}

// Define SLIP descriptor
slip_descriptor_s slip_descriptor = {
    .buf = buf,
    .buf_size = sizeof(buf),
    .crc_seed = 0xFFFF,
    .recv_message = recv_message,
    .write_byte = write_byte,
};

// SLIP handler instance
slip_handler_s slip;

void slip_init_handler() {
    slip_init(&slip, &slip_descriptor);
}

void slip_process_byte(uint8_t byte) {
    slip_read_byte(&slip, byte);
}
