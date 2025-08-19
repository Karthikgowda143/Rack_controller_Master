#ifndef SENSOR_HANDLER_H
#define SENSOR_HANDLER_H


#include "main.h"
#include "DHT.h"
#include "clock.h"
#include "math.h"
#include <SparkFunTMP102.h>
#include "local_storage_handler.h"

extern bool sdMounted;
bool extern factorymode; 
bool extern system_status_flag;
bool extern virginmodeflag;
bool extern startStopButtonPressed;

#define SDA_PIN 8        // I2C SDA pin
#define SCL_PIN 9        // I2C SCL pin
#define DHT11_PIN 3      // DHT11 data pin
#define SMOKE_SENSOR_PIN 1
#define BATTERY_VOLTAGE_PIN 2
#define FAN1_PIN 5
#define FAN2_PIN 6
#define FAN3_PIN 7
#define FAN4_PIN 14  
#define FAN5_PIN 21
#define FAN6_PIN 36
#define FAN7_PIN 37
#define FAN8_PIN 38

#define VOLTAGE_RANGE1_MIN 24
#define VOLTAGE_RANGE1_MAX 27
#define VOLTAGE_RANGE2_MIN 48
#define VOLTAGE_RANGE2_MAX 53
#define VOLTAGE_RANGE3_MIN 110
#define VOLTAGE_RANGE3_MAX 128

extern float temperature;
extern float humidity;
extern int smokeValue;
extern int batteryVoltage;
extern int normal_fanarray[4];  
extern int standby_fanarray[4];  
extern int doorarray[4];  
extern int indicators[8];

extern int rackID;
extern int password;

extern int tempThreshold;
extern int smokeThreshold;
extern int voltageRange;
extern int humidity_enable;
extern int four_g_enable;
extern int voltageRange_min;
extern int voltageRange_max;
extern bool sdMounted;

void SensorManagementTask(void *pvParameters);
void temp_sensor_Init();
void read_DHT11();
void read_smoke();
void readBatteryVoltage();
void i2c_init_handler();
void printInternalRTC();
void gpio_Init();
void normalFanOn();
void normalFanOff();
void standByFanOn();
void standByFanOff();

void set_voltage_range();
void check_emergency();
void printsensorData();
void sendAlertMessage();

#endif
