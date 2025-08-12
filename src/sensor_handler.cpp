#include "sensor_handler.h"

//Define the DHT11 sensor pin and type
#define DHTPIN 3      // DHT11 data pin
#define DHTTYPE DHT11   // Sensor type: DHT11
DHT dht(DHTPIN, DHTTYPE);

bool system_status_flag = false;
bool virginmodeflag = true;
bool startStopButtonPressed = true; //false; for testing
float temperature = 0.0;
float humidity = 0.0;
int smokeValue = 0;
int batteryVoltage = 24;

int normal_fanarray[4] = {2,2,2,2};  
int standby_fanarray[4] = {2,2,2,2};
int doorarray[4] = {2,2,2,2};
int indicators[8] = {0};

int tempThreshold = 0;  
int smokeThreshold = 0;   
int voltageRange = 0;
int humidity_enable = 0;  
int four_g_enable = 0;  
int voltageRange_min = 0; 
int voltageRange_max = 0;  

int rackID = 00;  
int password = 0000;  
unsigned long lastMillis = 0;

void SensorManagementTask(void *pvParameters) {

    delay(500);
    loadAllSettings();
    set_voltage_range();

    gpio_Init();

    i2c_init_handler(); 
    Serial.println("RTC initializing...");
    rtc_Init();
    // Set RTC time
    //rtc_SetTime(25, 06, 26, 15, 15, 0);  //for testing purposes
    Time t = rtc_GetTime();
    Serial.printf("System time: %04d-%02d-%02d %02d:%02d:%02d\n\n", t.year + 2000, t.month, t.day, t.hour, t.minute, t.second);
    
    temp_sensor_Init();
    dht.begin();  

    if(virginmodeflag == false && startStopButtonPressed == true) {
        normalFanOn();
    } else {
        normalFanOff();
    }

    setupLocalStorage(); // Initialize local storage for logging

  while (1) {
    
    if(virginmodeflag == false && startStopButtonPressed == true) {
        read_DHT11();  
        read_smoke();  
        readBatteryVoltage();   
        printsensorData(); 
    
        unsigned long currentMillis = millis();
        if (currentMillis - lastMillis >= 15000) {
            lastMillis = currentMillis;
            check_emergency();
        }
    }
    else {
        temperature = 0;
        humidity = 0;
        smokeValue = 0;
        batteryVoltage = 0;
    }
    vTaskDelay(pdMS_TO_TICKS(3000)); 
  }
}

void printsensorData() {
    Serial.println("============[SENSORS DATA]===============");
    Serial.printf("Rack ID           : %d                \n", rackID);
    Serial.printf("Temperature       : %f °C             \n", temperature);
    Serial.printf("Humidity          : %f %%            \n", humidity);
    Serial.printf("Smoke             : %d PPM           \n", smokeValue);
    Serial.printf("Battery Voltage   : %d V              \n", batteryVoltage);
    Serial.printf("Date and Time     : %04d-%02d-%02d %02d:%02d:%02d\n",
                   rtc_GetTime().year + 2000, rtc_GetTime().month, rtc_GetTime().day,
                   rtc_GetTime().hour, rtc_GetTime().minute, rtc_GetTime().second);

    Serial.printf("Normal Fan Status : [%d, %d, %d, %d]        \n",normal_fanarray[0],normal_fanarray[1],normal_fanarray[2],normal_fanarray[3]);
    
    Serial.printf("Standby Fan Status: [%d, %d, %d, %d]        \n",standby_fanarray[0],standby_fanarray[1],standby_fanarray[2],standby_fanarray[3]);
    
    Serial.printf("Door Status       : [%d, %d, %d, %d]         \n", doorarray[0], doorarray[1], doorarray[2], doorarray[3]);
    
    Serial.println("=======================================");
}

void check_emergency() {
    static bool smokeLogged = false;
    static bool tempLogged = false;
    static bool lowVoltLogged = false;
    static bool highVoltLogged = false;
    char buf[32];

    if (smokeValue > smokeThreshold) {
        Serial.println("Emergency! Smoke detected!");
        indicators[3] = 1;
        DynamicJsonDocument doc(256);
        doc["type"] = "emergency";
        doc["status"] = 0;
        char jsonBuffer[256];
        serializeJson(doc, jsonBuffer);
        slip_send_message(&slip, (uint8_t *)jsonBuffer, strlen(jsonBuffer));
        snprintf(buf, sizeof(buf), "Smoke Limit: %d PPM", smokeThreshold);
        if (!smokeLogged) {
            logData("EVENT", "SMOKE_ALERT", buf);
            smokeLogged = true;
        }
    } else {
        smokeLogged = false;
    }

    if (temperature > tempThreshold) {
        Serial.println("Emergency! High temperature detected!");
        indicators[4] = 1;
        DynamicJsonDocument doc(256);
        doc["type"] = "emergency";
        doc["status"] = 1;
        char jsonBuffer[256];
        serializeJson(doc, jsonBuffer);
        slip_send_message(&slip, (uint8_t *)jsonBuffer, strlen(jsonBuffer));
        snprintf(buf, sizeof(buf), "Temperature Limit: %d °C", tempThreshold);
        if (!tempLogged) {
            logData("EVENT", "TEMP_ALERT", buf);
            tempLogged = true;
        }
    } else {
        tempLogged = false;
    }

    if (batteryVoltage < voltageRange_min) {
        Serial.println("Emergency! Battery voltage out of range!");
        DynamicJsonDocument doc(256);
        doc["type"] = "emergency";
        doc["status"] = 2;
        char jsonBuffer[256];
        serializeJson(doc, jsonBuffer);
        slip_send_message(&slip, (uint8_t *)jsonBuffer, strlen(jsonBuffer));
        snprintf(buf, sizeof(buf), "Voltage range:[%d-%d] V", voltageRange_min, voltageRange_max);
        if (!lowVoltLogged) {
            logData("EVENT", "LOW_VOLTAGE", buf);
            lowVoltLogged = true;
        }
    } else {
        lowVoltLogged = false;
    }

    if (batteryVoltage > voltageRange_max) {
        Serial.println("Emergency! Battery voltage too high!");
        DynamicJsonDocument doc(256);
        doc["type"] = "emergency";
        doc["status"] = 3;
        char jsonBuffer[256];
        serializeJson(doc, jsonBuffer);
        slip_send_message(&slip, (uint8_t *)jsonBuffer, strlen(jsonBuffer));
        snprintf(buf, sizeof(buf), "Voltage range:[%d-%d] V", voltageRange_min, voltageRange_max);
        if (!highVoltLogged) {
            logData("EVENT", "HIGH_VOLTAGE", buf);
            highVoltLogged = true;
        }
    } else {
        highVoltLogged = false;
    }

    if (!(smokeValue > smokeThreshold) && !(temperature > tempThreshold) &&
        !(batteryVoltage < voltageRange_min) && !(batteryVoltage > voltageRange_max)) {
        Serial.println("No emergency");
        indicators[3] = 0;
        indicators[4] = 0;
        DynamicJsonDocument doc(256);
        doc["type"] = "emergency";
        doc["status"] = 108;
        char jsonBuffer[256];
        serializeJson(doc, jsonBuffer);
        slip_send_message(&slip, (uint8_t *)jsonBuffer, strlen(jsonBuffer));
    }
}


#define TMP102_ADDR 0x48
TMP102 sensor;

int retryCount = 0;
const int maxRetries = 4;

void temp_sensor_Init(){

    Wire.begin();
    while (!sensor.begin()) {
        Serial.println("Error: TMP102 not detected. Check connections!");
        // while (1);
        retryCount++;
        if (retryCount >= maxRetries) {
            Serial.println("Failed to initialize TMP102 after multiple attempts. Check connections!");
            break; // Exit the loop after max retries
        }
        delay(1000); // Wait before retrying
    }

    if (retryCount < maxRetries){
    // Configure sensor settings
    sensor.setFault(0);                // Trigger alarm immediately on fault
    sensor.setAlertPolarity(1);        // Actives HIGH alert - The ALERT pin will go HIGH (3.3V) when the alert condition is triggered.
    sensor.setAlertMode(0);            // Comparator Mode - The ALERT pin remains active as long as the temperature is above the T_HIGH threshold or below the T_LOW threshold.
    sensor.setConversionRate(2);       // Conversion rate: 4Hz
    sensor.setExtendedMode(0);         // Standard temperature range
   // sensor.setHighTempC(HIGHER_LIMIT);         // Upper limit in Celsius
    sensor.setLowTempC(0.0);          // Lower limit in Celsius
    }
}


void read_DHT11() {
    humidity = dht.readHumidity();
   // temperature = dht.readTemperature();// Celsius
    temperature = sensor.readTempC();

    if (isnan(humidity) || isnan(temperature)) {
        Serial.println("Failed to read from DHT sensor!");
        return;
    }

    if(temperature > tempThreshold) standByFanOn();
    else standByFanOff();  
}


void read_smoke() {
    smokeValue = analogRead(SMOKE_SENSOR_PIN);
    if (smokeValue > smokeThreshold) {
        Serial.println("Smoke Detected!");
        // Trigger buzzer or alert

        if(four_g_enable == 1) sendAlertMessage();
    } 
}

void sendAlertMessage() {
    StaticJsonDocument<256> doc;
    doc["type"] = "alert";
    doc["rack"] = rackID;
    doc["message"] = "Smoke detected! Immediate attention required.";
    doc["smoke_level"] = smokeValue;
    doc["timestamp"] = getFormattedTime();
    
    serializeJson(doc, Serial0);
    Serial0.println();
}


void readBatteryVoltage() {
   // batteryVoltage = analogRead(BATTERY_VOLTAGE_PIN); 
   batteryVoltage = 24;
}

void gpio_Init() {
    // Initialize GPIO pins
    pinMode(DHTPIN, INPUT);  // DHT11 data pin
    pinMode(SMOKE_SENSOR_PIN, INPUT);
    
    pinMode(FAN1_PIN, OUTPUT);
    pinMode(FAN2_PIN, OUTPUT);
    pinMode(FAN3_PIN, OUTPUT);
    pinMode(FAN4_PIN, OUTPUT);
    
    pinMode(FAN5_PIN, OUTPUT);
    pinMode(FAN6_PIN, OUTPUT);
    pinMode(FAN7_PIN, OUTPUT);
    pinMode(FAN8_PIN, OUTPUT);

    //start with low state
    digitalWrite(FAN1_PIN, HIGH);
    digitalWrite(FAN2_PIN, HIGH);
    digitalWrite(FAN3_PIN, HIGH);
    digitalWrite(FAN4_PIN, HIGH);
    digitalWrite(FAN5_PIN, HIGH);
    digitalWrite(FAN6_PIN, HIGH);
    digitalWrite(FAN7_PIN, HIGH);
    digitalWrite(FAN8_PIN, HIGH);
}
void normalFanOn() {
    for(int i = 0; i < 4; i++) {
        normal_fanarray[i] = 1; 
    }
    indicators[0] = 1;
    digitalWrite(FAN1_PIN, LOW);
    digitalWrite(FAN2_PIN, LOW);
    digitalWrite(FAN3_PIN, LOW);
    digitalWrite(FAN4_PIN, LOW);
}

void normalFanOff() {
    for(int i = 0; i < 4; i++) {
        normal_fanarray[i] = 2;
    }
    indicators[0] = 0;
    digitalWrite(FAN1_PIN, HIGH);
    digitalWrite(FAN2_PIN, HIGH);
    digitalWrite(FAN3_PIN, HIGH);
    digitalWrite(FAN4_PIN, HIGH);
}

void standByFanOn() {
    for(int i = 0; i < 4; i++) {
        standby_fanarray[i] = 1; 
    }
    indicators[1] = 1;
    digitalWrite(FAN5_PIN, LOW);
    digitalWrite(FAN6_PIN, LOW);
    digitalWrite(FAN7_PIN, LOW);
    digitalWrite(FAN8_PIN, LOW);
}

void standByFanOff() {
    for(int i = 0; i < 4; i++) {
        standby_fanarray[i] = 2; 
    }
    indicators[1] = 0;
    digitalWrite(FAN5_PIN, HIGH);
    digitalWrite(FAN6_PIN, HIGH);
    digitalWrite(FAN7_PIN, HIGH);
    digitalWrite(FAN8_PIN, HIGH);
}

// Function to initialize I2C for sensors
void i2c_init_handler() {
     Wire.begin(SDA_PIN, SCL_PIN);
    // Scan I2C bus for devices
    Serial.println("\nScanning for I2C devices...");
    byte error, address;
    int nDevices = 0;
  
    for (address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
      
        if (error == 0) {
            Serial.print("I2C device found at 0x");
            Serial.println(address, HEX);
            nDevices++;
        }
    }
    if (nDevices == 0) Serial.println("No I2C devices found.\n");
    else Serial.println("I2C scan complete.\n");
}

void set_voltage_range() {
    if (voltageRange == 1) {
        voltageRange_min = VOLTAGE_RANGE1_MIN;
        voltageRange_max = VOLTAGE_RANGE1_MAX;
    } else if (voltageRange == 2) {
        voltageRange_min = VOLTAGE_RANGE2_MIN;
        voltageRange_max = VOLTAGE_RANGE2_MAX;
    } else if (voltageRange == 3) {
        voltageRange_min = VOLTAGE_RANGE3_MIN;
        voltageRange_max = VOLTAGE_RANGE3_MAX;
    } else {
        voltageRange_min = 0;  // Default to 0 if no valid range is selected
        voltageRange_max = 0;
    }
    Serial.printf("Voltage Range set: %d - %d\n", voltageRange_min, voltageRange_max);
}