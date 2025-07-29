#ifndef SPIFFS_HANDLER_H
#define SPIFFS_HANDLER_H

#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "sensor_handler.h"

void saveConfigSettings(int tempThreshold, int smokeThreshold, int voltageRange, int humidity_enable, int four_g_enable);
void saveRackID(int value);
void savePassword(int value);
void loadAllSettings();
void saveSlave_virgin(bool virginmode);
void saveButtonStatus(bool startStopButtonPressed);

#endif