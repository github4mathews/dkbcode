#ifndef SPIFFSSERVICES_H
#define SPIFFSSERVICES_H

#include <Arduino.h>
#include <EEPROM.h>
#include <String.h>
#include <SPIFFS.h>
#include <utlityServices.h>
#include <iostream>
using namespace std;
#include <ArduinoJson.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>

class SpiffsServices
{
public:
    bool getDeviceData(String &infojson, UtlityServices utlityServices, String filePath);
    void ConfigFile_Save_Variable(String From, String VarName, String VarValue, UtlityServices utlityService);
    void updateReciverListIfNotFound(String ListName, String mac, String &infojsonAsString, bool EnableDebug, UtlityServices utlityService);

private:
};

#endif