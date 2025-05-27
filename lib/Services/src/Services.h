#ifndef SERVICES_H
#define SERVICES_H

#include <Arduino.h>
#include <EEPROM.h>
#include <String.h>
#include <utilServices.h>
#include <iostream>
using namespace std;
#include <ArduinoJson.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include "FS.h"
// #include "SPIFFS.h"
#include "LittleFS.h"

#define SPIFFS LittleFS

/* This examples uses "quick re-define" of SPIFFS to run
   an existing sketch with LittleFS instead of SPIFFS

   You only need to format LittleFS the first time you run a
   test or else use the LittleFS plugin to create a partition
   https://github.com/lorol/arduino-esp32littlefs-plugin */

#define FORMAT_LITTLEFS_IF_FAILED true

class Services
{
public:
    bool getDeviceData(String &infojson, UtilServices utilServices, String filePath);
    void ConfigFile_Save_Variable(String From, String VarName, String VarValue, UtilServices utilServices);
    void updateReciverListIfNotFound(String ListName, String mac, String &infojsonAsString, bool EnableDebug, UtilServices utilServices);

private:
};

#endif