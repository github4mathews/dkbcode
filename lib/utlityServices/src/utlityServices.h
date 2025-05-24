#ifndef UTLITYSERVICES_H
#define UTLITYSERVICES_H

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

class UtlityServices
{
public:
    void printLine(String message);
    void printDash(int count);
    void printInitializingDebug(String reciverList, bool peerAdded, uint8_t broadcastAddress[6], int WorkingMode);
    void printOutgoingMessage(String outgoingJSON);

private:
};

#endif