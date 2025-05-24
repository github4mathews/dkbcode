#ifndef STRINGHANDLINGSERVICES_H
#define STRINGHANDLINGSERVICES_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <String.h>
#include <SPIFFS.h>
#include <iostream>
using namespace std;
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>

class StringHandlingServices
{
public:
    void str_replace(char *src, char *oldchars, char *newchars);
    int StringSplit(String sInput, char cDelim, String sParams[], int iMaxParams);

private:
};

#endif