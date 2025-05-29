#ifndef UTLITYSERVICES_H
#define UTLITYSERVICES_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <String.h>
#include <iostream>
using namespace std;
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
// #include "FS.h"
// #include "SPIFFS.h"
#include <LittleFS.h>

// #define SPIFFS LittleFS

/* This examples uses "quick re-define" of SPIFFS to run
   an existing sketch with LittleFS instead of SPIFFS

   You only need to format LittleFS the first time you run a
   test or else use the LittleFS plugin to create a partition
   https://github.com/lorol/arduino-esp32littlefs-plugin */

// #define FORMAT_LITTLEFS_IF_FAILED true

// #define SPIFFS LittleFS

/* This examples uses "quick re-define" of SPIFFS to run
   an existing sketch with LittleFS instead of SPIFFS

   You only need to format LittleFS the first time you run a
   test or else use the LittleFS plugin to create a partition
   https://github.com/lorol/arduino-esp32littlefs-plugin */

// #define FORMAT_LITTLEFS_IF_FAILED true

class UtilServices
{
public:
    void printLine(String message);
    void printDash(int count);
    void printInitializingDebug(String reciverList, bool peerAdded, uint8_t broadcastAddress[6], int WorkingMode);
    void printOutgoingMessage(String outgoingJSON);

private:
};

#endif