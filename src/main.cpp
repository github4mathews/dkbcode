#define ARDUINOJSON_ENABLE_COMMENTS 1
#define DEBUG

#include <Arduino.h>
#include <ArduinoJson.h>
// #include <EEPROM.h>
// #include <String.h>
#include <Services.h>
#include <UtilServices.h>
#include <StrHandlingServices.h>
// #include <iostream>
// #include <SoftwareSerial.h>
// #include <esp_now.h>
// #include <esp_wifi.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <AsyncTCP.h>
#include <Update.h>
#include <esp32/spiram.h>
// #include "FS.h"
// #include "LittleFS.h"
#include <LittleFS.h> // Ensure you're using LittleFS, not LittleFS

// #define LittleFS LittleFS

/* This examples uses "quick re-define" of LittleFS to run
   an existing sketch with LittleFS instead of LittleFS

   You only need to format LittleFS the first time you run a
   test or else use the LittleFS plugin to create a partition
   https://github.com/lorol/arduino-esp32littlefs-plugin */

// #define FORMAT_LITTLEFS_IF_FAILED true
using namespace std;

int wifiChannel = 10;

WiFiClientSecure espClient;
PubSubClient client(espClient);

AsyncWebServer server(80);
AsyncEventSource events("/events");
unsigned long lastEvent = 0;

Services service = Services();
UtilServices utilServices = UtilServices();
StrHandlingServices stringHandlingService = StrHandlingServices();

JsonDocument infojson;

String mqtt_broker;
String ca_cert;
String user_name;
String password;

// ==========================  WEB HANDLING    ===================================
const char *BSSID = "bssid";
const char *PASS = "password";
const char *SENSOR = "sensor";

// Global variable to track scan status
int wifiScanStatus = -1;
DynamicJsonDocument wifiScanResults(1024);

void notFound(AsyncWebServerRequest *request);
void HandleWeb();
String getParameter(AsyncWebServerRequest *request);
int32_t getWiFiChannel(const char *ssid);
void setup_wifi();
void reconnect();
void MQTT_CB(char *topic, byte *payload, unsigned int length);
void printLine(String str);

bool isValidPath(const String &filename)
{
  return !filename.isEmpty() &&
         filename[0] == '/' &&
         filename.indexOf("..") == -1 &&
         filename.indexOf("//") == -1;
}

// Helper: Load known Wi-Fi from info.json
bool loadKnownWifi(JsonArray &knownArr, String &currentSSID, String &currentPWD)
{
  File file = LittleFS.open("/info.json", "r");

  if (!file)
    return false;
  DynamicJsonDocument doc(1024);
  DeserializationError err = deserializeJson(doc, file);
  file.close();
  if (err)
    return false;

  // Check if KNOWN exists and is an array
  // if (doc["WIFI"]["KNOWN"].isNull())
  //   return false;

  if (!doc["WIFI"].containsKey("KNOWN") || !doc["WIFI"]["KNOWN"].is<JsonArray>())
  {
    Serial.println("No known WiFi networks in info.json!");
    return false;
  }
  JsonArray knownArr2 = doc["WIFI"]["KNOWN"].as<JsonArray>();
  if (knownArr2.size() == 0)
  {
    Serial.println("KNOWN array is empty!");
    return false;
  }
  knownArr = doc["WIFI"]["KNOWN"].as<JsonArray>();
  currentSSID = doc["WIFI"]["CURRENT"]["SSID"] | "";
  currentPWD = doc["WIFI"]["CURRENT"]["PWD"] | "";
  return true;
}

// Helper: Save current Wi-Fi to info.json
void saveCurrentWifi(const String &ssid, const String &pwd)
{
  File file = LittleFS.open("/info.json", "r");
  if (!file)
    return;
  DynamicJsonDocument doc(1024);
  DeserializationError err = deserializeJson(doc, file);
  file.close();
  if (err)
    return;
  doc["WIFI"]["CURRENT"]["SSID"] = ssid;
  doc["WIFI"]["CURRENT"]["PWD"] = pwd;
  file = LittleFS.open("/info.json", "w");
  serializeJson(doc, file);
  file.close();
}

void autoConnectKnownWifi()
{
  File file = LittleFS.open("/info.json", "r");
  if (!file)
  {
    Serial.println("Failed to open info.json");
    return;
  }
  DynamicJsonDocument doc(1024);
  DeserializationError err = deserializeJson(doc, file);
  file.close();
  if (err)
  {
    Serial.println("Failed to parse info.json");
    return;
  }
  if (!doc["WIFI"].containsKey("KNOWN") || !doc["WIFI"]["KNOWN"].is<JsonArray>())
  {
    Serial.println("No known WiFi networks in info.json!");
    return;
  }
  JsonArray knownArr = doc["WIFI"]["KNOWN"].as<JsonArray>();
  if (knownArr.size() == 0)
  {
    Serial.println("KNOWN array is empty!");
    return;
  }

  int n = WiFi.scanNetworks();
  int bestIdx = -1;
  int bestRSSI = -1000;
  String bestSSID, bestPWD;

  for (int i = 0; i < n; ++i)
  {
    String foundSSID = WiFi.SSID(i);
    for (size_t j = 0; j < knownArr.size(); ++j)
    {
      String knownSSID = knownArr[j]["SSID"].as<String>();
      if (foundSSID == knownSSID)
      {
        int rssi = WiFi.RSSI(i);
        if (rssi > bestRSSI)
        {
          bestRSSI = rssi;
          bestSSID = foundSSID;
          bestPWD = knownArr[j]["PWD"].as<String>();
          bestIdx = j;
        }
      }
    }
  }
  if (bestIdx != -1)
  {
    Serial.printf("Connecting to best known SSID: %s\n", bestSSID.c_str());
    WiFi.begin(bestSSID.c_str(), bestPWD.c_str());
    IPAddress dns(8, 8, 8, 8); // Google DNS
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, dns);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000)
    {
      delay(500);
    }
    if (WiFi.status() == WL_CONNECTED)
    {
      // Save CURRENT
      File fileW = LittleFS.open("/info.json", "r");
      if (!fileW)
        return;
      DynamicJsonDocument docW(1024);
      DeserializationError errW = deserializeJson(docW, fileW);
      fileW.close();
      if (errW)
        return;
      docW["WIFI"]["CURRENT"]["SSID"] = bestSSID;
      docW["WIFI"]["CURRENT"]["PWD"] = bestPWD;
      fileW = LittleFS.open("/info.json", "w");
      serializeJson(docW, fileW);
      fileW.close();

      randomSeed(micros());
      Serial.println((String) "\nWiFi connected\nIP address: " + WiFi.localIP());
      Serial.print("Station IP Address: ");
      Serial.println(WiFi.localIP());
      Serial.print("Wi-Fi Channel: ");
      Serial.println(WiFi.channel());
    }
  }
  else
  {
    Serial.println("No known WiFi found nearby.");
  }
}

void startWifiScan()
{
  wifiScanStatus = WiFi.scanNetworks(true); // true = async
}

void printLine(String message)
{
  Serial.println();

  for (size_t i = 0; i < 10; i++)
    Serial.print("=");

  Serial.print(message);

  for (size_t i = 0; i < 10; i++)
    Serial.print("=");

  Serial.println();
}

void MQTT_CB(char *topic, byte *payload, unsigned int length)
{
  Serial.println((String) "\nMQTT(MA) Topic: " + topic);
  String inputString = "";
  for (int i = 0; i < length; i++)
  {
    char inChar = (char)payload[i];
    inputString += inChar;
  }
}

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

String getParameter(AsyncWebServerRequest *request)
{
  bool _bssid;
  bool _password;
  bool _sensor;

  String bssid;
  String password;
  String sensor;

  if (request->hasParam(BSSID, true))
  {
    bssid = request->getParam(BSSID, true)->value();
    _bssid = true;
  }
  else
  {
    _bssid = false;
  }

  if (request->hasParam(PASS, true))
  {
    password = request->getParam(PASS, true)->value();
    _password = true;
  }
  else
  {
    _password = false;
  }

  if (request->hasParam(SENSOR, true))
  {
    sensor = request->getParam(SENSOR, true)->value();
    _sensor = true;
  }
  else
  {
    _sensor = false;
  }

  if (_bssid == true && _password == true && _sensor == true)
  {
    return "Bssid: " + bssid + " Pass: " + password + " Sensor: " + sensor;
  }
  else
  {
    String conformation = "";

    if (_bssid != true)
      conformation += "BSSID, ";
    if (_password != true)
      conformation += "Password,";
    if (_sensor != true)
      conformation += "Sensor ";

    return conformation += " is missing.";
  }
}

// Helper: Load users array from info.json

bool loadUsers(JsonDocument &doc, JsonArray &users)
{
  File file = LittleFS.open("/info.json", "r");
  if (!file)
    return false;
  DeserializationError err = deserializeJson(doc, file);
  file.close();
  if (err)
    return false;
  if (!doc.containsKey("USERS") || !doc["USERS"].is<JsonArray>())
  {
    doc["USERS"] = JsonArray();
  }
  users = doc["USERS"].as<JsonArray>();
  return true;
}

// Helper: Save users array to info.json
bool saveUsers(JsonDocument &doc)
{
  File file = LittleFS.open("/info.json", "w");
  if (!file)
    return false;
  serializeJson(doc, file);
  file.close();
  return true;
}

void HandleWeb()
{
  // Serve dashboard HTML
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/index.html", "text/html"); });
  server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/index.html", "text/html"); });
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/favicon.ico", "image/x-icon"); });
  server.on("/folder-icon.png", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/folder-icon.png", "image/x-icon"); });
  server.on("/file-icon.png", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/file-icon.png", "image/x-icon"); });
  server.on("/login.html", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/login.html", "text/html"); });
  server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/styles.css", "text/css"); });
  // Serve JS
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/script.js", "text/javascript"); });
  server.on("/logo.svg", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/logo.svg", "image/svg+xml"); });
  server.on("/profile.png", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/profile.png", "image/png"); });
  server.on("/fmstyles.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/fmstyles.css", "text/css"); });
  server.on("/fmscript.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/fmscript.js", "text/javascript"); });
  server.on("/filemanager.html", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/filemanager.html", "text/html"); });

  // SSE endpoint
  events.onConnect([](AsyncEventSourceClient *client)
                   { Serial.println("Client connected to events"); });
  server.addHandler(&events);

  // CREATE user (register)
  server.on("/users", HTTP_POST, [](AsyncWebServerRequest *request)
            {
      if (request->hasParam("username", true) && request->hasParam("password", true)) {
          String username = request->getParam("username", true)->value();
          String password = request->getParam("password", true)->value();
          DynamicJsonDocument doc(1024);
          JsonArray users;
          if (!loadUsers(doc, users)) {
              request->send(500, "application/json", "{\"status\":\"fail_load\"}");
              return;
          }
          for (JsonObject user : users) {
              if (user["USER_NAME"] == username) {
                  request->send(409, "application/json", "{\"status\":\"user_exists\"}");
                  return;
              }
          }
          JsonObject newUser = users.createNestedObject();
          newUser["USER_NAME"] = username;
          newUser["PASSWORD"] = password;
          if (saveUsers(doc)) {
              request->send(200, "application/json", "{\"status\":\"success\"}");
          } else {
              request->send(500, "application/json", "{\"status\":\"fail_save\"}");
          }
      } else {
          request->send(400, "application/json", "{\"status\":\"missing_fields\"}");
      } });

  // READ all users
  server.on("/users", HTTP_GET, [](AsyncWebServerRequest *request)
            {
      DynamicJsonDocument doc(1024);
      JsonArray users;
      if (!loadUsers(doc, users)) {
          request->send(500, "application/json", "{\"status\":\"fail_load\"}");
          return;
      }
      String result;
      serializeJson(users, result);
      request->send(200, "application/json", result); });

  // UPDATE user password
  server.on("/users", HTTP_PUT, [](AsyncWebServerRequest *request)
            {
      if (request->hasParam("username", true) && request->hasParam("password", true)) {
          String username = request->getParam("username", true)->value();
          String password = request->getParam("password", true)->value();
          DynamicJsonDocument doc(1024);
          JsonArray users;
          if (!loadUsers(doc, users)) {
              request->send(500, "application/json", "{\"status\":\"fail_load\"}");
              return;
          }
          bool found = false;
          for (JsonObject user : users) {
              if (user["USER_NAME"] == username) {
                  user["PASSWORD"] = password;
                  found = true;
                  break;
              }
          }
          if (!found) {
              request->send(404, "application/json", "{\"status\":\"not_found\"}");
              return;
          }
          if (saveUsers(doc)) {
              request->send(200, "application/json", "{\"status\":\"success\"}");
          } else {
              request->send(500, "application/json", "{\"status\":\"fail_save\"}");
          }
      } else {
          request->send(400, "application/json", "{\"status\":\"missing_fields\"}");
      } });

  // DELETE user
  server.on("/users", HTTP_DELETE, [](AsyncWebServerRequest *request)
            {
      if (request->hasParam("username", true)) {
          String username = request->getParam("username", true)->value();
          DynamicJsonDocument doc(1024);
          JsonArray users;
          if (!loadUsers(doc, users)) {
              request->send(500, "application/json", "{\"status\":\"fail_load\"}");
              return;
          }
          bool found = false;
          for (size_t i = 0; i < users.size(); ++i) {
              if (users[i]["USER_NAME"] == username) {
                  users.remove(i);
                  found = true;
                  break;
              }
          }
          if (!found) {
              request->send(404, "application/json", "{\"status\":\"not_found\"}");
              return;
          }
          if (saveUsers(doc)) {
              request->send(200, "application/json", "{\"status\":\"success\"}");
          } else {
              request->send(500, "application/json", "{\"status\":\"fail_save\"}");
          }
      } else {
          request->send(400, "application/json", "{\"status\":\"missing_fields\"}");
      } });

  // Start Wi-Fi scan (async)
  server.on("/wifi/scan/start", HTTP_GET, [](AsyncWebServerRequest *request)
            {
      startWifiScan();
      request->send(200, "application/json", "{\"status\":\"scanning\"}"); });

  // Get Wi-Fi scan results
  server.on("/wifi/scan", HTTP_GET, [](AsyncWebServerRequest *request)
            {
        int n = WiFi.scanComplete();
        if (n == WIFI_SCAN_RUNNING) {
            request->send(200, "application/json", "{\"status\":\"scanning\"}");
            return;
        } else if (n < 0) {
            request->send(200, "application/json", "[]");
            return;
        }
        wifiScanResults.clear();
        JsonArray arr = wifiScanResults.to<JsonArray>();
        for (int i = 0; i < n; ++i) {
            JsonObject net = arr.createNestedObject();
            net["ssid"] = WiFi.SSID(i);
            net["rssi"] = WiFi.RSSI(i);
            net["secure"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
        }
        WiFi.scanDelete();
        String result;
        serializeJson(arr, result);
        request->send(200, "application/json", result); });

  // Save Wi-Fi credentials to info.json
  server.on("/wifi/save", HTTP_POST, [](AsyncWebServerRequest *request)
            {
    if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
        String ssid = request->getParam("ssid", true)->value();
        String password = request->getParam("password", true)->value();

        Serial.println((String) "Saving Wi-Fi credentials: SSID: " + ssid + ", Password: " + password);

        File file = LittleFS.open("/info.json", "r");
        if (!file) {
            request->send(500, "application/json", "{\"status\":\"fail_open\"}");
            return;
        }
        DynamicJsonDocument doc(1024);
        DeserializationError err = deserializeJson(doc, file);
        file.close();
        if (err) {
            request->send(500, "application/json", "{\"status\":\"fail_parse\"}");
            return;
        }

        // Add or update in KNOWN array
        bool found = false;
        JsonArray knownArr = doc["WIFI"]["KNOWN"].to<JsonArray>();
        for (JsonObject net : knownArr) {
            if (net["SSID"] == ssid) {
                net["PWD"] = password;
                found = true;
                break;
            }
        }
        if (!found) {
            JsonObject newNet = knownArr.createNestedObject();
            newNet["SSID"] = ssid;
            newNet["PWD"] = password;
        }
        // Update CURRENT
        doc["WIFI"]["CURRENT"]["SSID"] = ssid;
        doc["WIFI"]["CURRENT"]["PWD"] = password;

        file = LittleFS.open("/info.json", "w");
        serializeJson(doc, file);
        file.close();
        request->send(200, "application/json", "{\"status\":\"success\"}");
    } else {
        request->send(400, "application/json", "{\"status\":\"missing_fields\"}");
    } });

  server.on("/wifi/known", HTTP_GET, [](AsyncWebServerRequest *request)
            {
      File file = LittleFS.open("/info.json", "r");
      if (!file) {
          request->send(404, "application/json", "{\"error\":\"info.json not found\"}");
          return;
      }
      DynamicJsonDocument doc(1024);
      DeserializationError err = deserializeJson(doc, file);
      file.close();
      if (err) {
          request->send(500, "application/json", "{\"error\":\"fail_parse\"}");
          return;
      }
      String result;
      serializeJson(doc["WIFI"]["KNOWN"], result);
      request->send(200, "application/json", result); });

  // Serve info.json as JSON
  server.on("/wifi/info", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    if (!LittleFS.exists("/info.json")) {
        request->send(404, "application/json", "{\"error\":\"info.json not found\"}");
        return;
    }
    File file = LittleFS.open("/info.json", "r");
    if (!file) {
        request->send(500, "application/json", "{\"error\":\"fail_open\"}");
        return;
    }
    DynamicJsonDocument doc(1024);
    DeserializationError err = deserializeJson(doc, file);
    file.close();
    if (err) {
        request->send(500, "application/json", "{\"error\":\"fail_parse\"}");
        return;
    }
    DynamicJsonDocument wifiDoc(256);
    wifiDoc["SSID"] = doc["WIFI"]["CURRENT"]["SSID"] | "";
    wifiDoc["PWD"] = doc["WIFI"]["CURRENT"]["PWD"] | "";
    String result;
    serializeJson(wifiDoc, result);
    request->send(200, "application/json", result); });

  // GET current MQTT info
  server.on("/mqtt/info", HTTP_GET, [](AsyncWebServerRequest *request)
            {
  File file = LittleFS.open("/info.json", "r");
  if (!file) {
      request->send(404, "application/json", "{\"error\":\"info.json not found\"}");
      return;
  }
  DynamicJsonDocument doc(1024);
  DeserializationError err = deserializeJson(doc, file);
  file.close();
  if (err) {
      request->send(500, "application/json", "{\"error\":\"fail_parse\"}");
      return;
  }
  String result;
  serializeJson(doc["MQTT"], result);
  request->send(200, "application/json", result); });

  // POST save MQTT info
  server.on("/mqtt/save", HTTP_POST, [](AsyncWebServerRequest *request)
            {
              // This will be empty; use onRequestBody below
            },
            NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            {
    static String body;
    if (index == 0) body = ""; // Start of new body
    body += String((const char*)data, len);
    if (index + len == total) { // All data received
      Serial.println(body);
      DynamicJsonDocument input(1024);
      DeserializationError err = deserializeJson(input, body);
      if (err)
      {
        request->send(400, "application/json", "{\"status\":\"fail_parse\"}");
        return;
      }
      File file = LittleFS.open("/info.json", "r");
      if (!file)
      {
        request->send(500, "application/json", "{\"status\":\"fail_open\"}");
        return;
      }
      DynamicJsonDocument doc(1024);
      DeserializationError err2 = deserializeJson(doc, file);
      file.close();
      if (err2)
      {
        request->send(500, "application/json", "{\"status\":\"fail_parse2\"}");
        return;
      }
        JsonObject mqtt = doc["MQTT"];
        mqtt["MQTT_SERVER"] = input["server"] | "";
        mqtt["DEFAULT_USERNAME"] = input["username"] | "";
        mqtt["DEFAULT_PWD"] = input["password"] | "";
        mqtt["ACL_CLIENT_ID"] = input["clientId"] | "";
        mqtt["MQ_PORT"] = input["port"] | 1883;
        mqtt["WS_PORT"] = input["wsPort"] | 8044;
        mqtt["PUBLISH_TOPIC"] = input["publishTopic"].isNull() ? JsonArray() : input["publishTopic"];
        mqtt["SUBSCRIBE_TOPIC"] = input["subscribeTopic"].isNull() ? JsonArray() : input["subscribeTopic"];
        mqtt["SERVER_CERTIFICATE"] = input["serverCert"] | "";

        file = LittleFS.open("/info.json", "w");
        serializeJson(doc, file);
        file.close();
        request->send(200, "application/json", "{\"status\":\"success\"}");
    } });

  // // List files
  // server.on("/file/list", HTTP_GET, [](AsyncWebServerRequest *request)
  //           {
  // String output = "[";
  // File root = LittleFS.open("/");
  // File file = root.openNextFile();
  // bool first = true;
  // while(file){
  //     if (!first) output += ",";
  //     output += "{\"name\":\"" + String(file.name()) + "\",\"size\":" + String(file.size()) + "}";
  //     Serial.println(output);
  //     Serial.println(file.name());
  //     file = root.openNextFile();
  //     first = false;
  // }
  // output += "]";
  // request->send(200, "application/json", output); });

  // // Download file
  // // server.on("/file", HTTP_GET, [](AsyncWebServerRequest *request)
  // //           {
  // // if (!request->hasParam("name")) {
  // //     request->send(400, "text/plain", "Missing file name");
  // //     return;
  // // }
  // // String filename = request->getParam("name")->value();
  // // if (!LITTLEFS.exists(filename)) {
  // //     request->send(404, "text/plain", "File not found");
  // //     return;
  // // }
  // // request->send(LITTLEFS, filename, "application/octet-stream", true); });
  // server.on("/file", HTTP_GET, [](AsyncWebServerRequest *request)
  //           {
  //   if (!request->hasParam("name")) {
  //       request->send(400, "text/plain", "Missing file name");
  //       return;
  //   }
  //   String filename = request->getParam("name")->value();
  //   if (!filename.startsWith("/")) filename = "/" + filename;
  //   Serial.print("Requested file: "); Serial.println(filename);
  //   if (!LittleFS.exists(filename)) {
  //       Serial.println("File not found in LittleFS!");
  //       request->send(404, "text/plain", "File not found");
  //       return;
  //   }
  //   request->send(LittleFS, filename, "application/octet-stream", true); });

  // // Delete file
  // server.on("/file", HTTP_DELETE, [](AsyncWebServerRequest *request)
  //           {
  // if (!request->hasParam("name")) {
  //     request->send(400, "text/plain", "Missing file name");
  //     return;
  // }
  // String filename = request->getParam("name")->value();
  // if (!LittleFS.exists(filename)) {
  //     request->send(404, "text/plain", "File not found");
  //     return;
  // }
  // LittleFS.remove(filename);
  // request->send(200, "text/plain", "File deleted"); });

  // // Upload file (POST, multipart/form-data)
  // server.on("/file", HTTP_POST, [](AsyncWebServerRequest *request)
  //           { request->send(200, "text/plain", "File uploaded"); }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
  //           {
  // static File uploadFile;
  // if(index == 0){
  //     uploadFile = LittleFS.open("/" + filename, "w");
  // }
  // if(uploadFile){
  //     uploadFile.write(data, len);
  //     if(final){
  //         uploadFile.close();
  //     }
  // } });

  //================================File Manager========================================
  // List files and folders
  server.on("/file/list", HTTP_GET, [](AsyncWebServerRequest *request)
            {
  String output = "[";
  File root = LittleFS.open("/");
  File file = root.openNextFile();
  bool first = true;
  while (file) {
      if (!first) output += ",";
      String fileName = String(file.name());
      bool isDir = fileName.endsWith("/.placeholder");
      if (isDir) {
          fileName = fileName.substring(0, fileName.lastIndexOf("/"));
      }
      output += "{\"name\":\"" + fileName + "\",\"size\":" + String(file.size()) + ",\"isDir\":" + (isDir ? "true" : "false") + "}";
      file = root.openNextFile();
      first = false;
  }
  output += "]";
  request->send(200, "application/json", output); });

  // Download file
  server.on("/file", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    if (!request->hasParam("name")) {
        request->send(400, "text/plain", "Missing file name");
        return;
    }
    String filename = request->getParam("name")->value();
    if (!filename.startsWith("/")) filename = "/" + filename;
    if (!isValidPath(filename)) {
        request->send(400, "text/plain", "Invalid file name");
        return;
    }
    if (!LittleFS.exists(filename))
    {
      request->send(404, "text/plain", "File not found");
      return;
    }
    request->send(LittleFS, filename, "application/octet-stream", true); });

  // Delete file/folder
  server.on("/file", HTTP_DELETE, [](AsyncWebServerRequest *request)
            {
    if (!request->hasParam("name")) {
        request->send(400, "text/plain", "Missing file name");
        return;
    }
    String filename = request->getParam("name")->value();
    if (!filename.startsWith("/")) filename = "/" + filename;
    if (!isValidPath(filename)) {
        request->send(400, "text/plain", "Invalid file name");
        return;
    }
    if (!LittleFS.exists(filename))
    {
      request->send(404, "text/plain", "File/folder not found");
      return;
    }
    if (LittleFS.remove(filename))
    {
      request->send(200, "text/plain", "Deleted");
    }
    else
    {
      request->send(500, "text/plain", "Delete failed");
    } });

  // Upload file
  server.on("/file", HTTP_POST, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", "File uploaded"); }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
            {
    static File uploadFile;
    if (index == 0) {
        if (!filename.startsWith("/")) filename = "/" + filename;
        if (!isValidPath(filename)) return;
        uploadFile = LittleFS.open(filename, "w");
    }
    if (uploadFile) {
        uploadFile.write(data, len);
        if (final) uploadFile.close();
    } });

  // Create folder (simulated using a placeholder file)
  server.on("/folder", HTTP_POST, [](AsyncWebServerRequest *request)
            {
  if (!request->hasParam("name")) {
      request->send(400, "text/plain", "Missing folder name");
      return;
  }
  String folderName = request->getParam("name")->value();
  if (!folderName.startsWith("/")) folderName = "/" + folderName;
  if (!isValidPath(folderName)) {
      request->send(400, "text/plain", "Invalid folder name");
      return;
  }
  // Simulate folder creation by creating a placeholder file
  String placeholderFile = "/" + folderName + "/.placeholder";
  File file = LittleFS.open(placeholderFile, "w");
  if (!file) {
      request->send(500, "text/plain", "Failed to create folder");
      return;
  }
  file.close();
  request->send(200, "text/plain", "Folder created"); });

  // Rename file/folder
  server.on("/file/rename", HTTP_PUT, [](AsyncWebServerRequest *request)
            {
    if (!request->hasParam("oldName") || !request->hasParam("newName")) {
        request->send(400, "text/plain", "Missing old or new name");
        return;
    }
    String oldName = request->getParam("oldName")->value();
    String newName = request->getParam("newName")->value();
    if (!oldName.startsWith("/")) oldName = "/" + oldName;
    if (!newName.startsWith("/")) newName = "/" + newName;
    if (!isValidPath(oldName) || !isValidPath(newName)) {
        request->send(400, "text/plain", "Invalid file/folder name");
        return;
    }
    if (!LittleFS.exists(oldName))
    {
      request->send(404, "text/plain", "Old file/folder not found");
      return;
    }
    if (LittleFS.rename(oldName, newName))
    {
      request->send(200, "text/plain", "Renamed");
    }
    else
    {
      request->send(500, "text/plain", "Rename failed");
    } });

  // Get file content for editing
  server.on("/file/content", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    if (!request->hasParam("name")) {
        request->send(400, "text/plain", "Missing file name");
        return;
    }
    String filename = request->getParam("name")->value();
    if (!filename.startsWith("/")) filename = "/" + filename;
    if (!isValidPath(filename)) {
        request->send(400, "text/plain", "Invalid file name");
        return;
    }
    if (!LittleFS.exists(filename))
    {
      request->send(404, "text/plain", "File not found");
      return;
    }
    File file = LittleFS.open(filename, "r");
    String content = file.readString();
    file.close();
    request->send(200, "text/plain", content); });

  // Edit file content
  server.on("/file/edit", HTTP_PUT, [](AsyncWebServerRequest *request)
            {
      if (!request->hasParam("name", true)) {
          request->send(400, "text/plain", "Missing file name");
          return;
      }
      String filename = request->getParam("name", true)->value();
      if (!filename.startsWith("/")) filename = "/" + filename;
      if (!isValidPath(filename)) {
          request->send(400, "text/plain", "Invalid file name");
          return;
      }
      request->send(200, "text/plain", "Ready to edit"); }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            {
      static File editFile;
      if (index == 0) {
          String filename = request->getParam("name", true)->value();
          if (!filename.startsWith("/")) filename = "/" + filename;
          if (!isValidPath(filename)) return;
          editFile = LittleFS.open(filename, "w");
          if (!editFile) {
              request->send(500, "text/plain", "Failed to open file for editing");
              return;
          }
      }
      if (editFile) {
          editFile.write(data, len);
          if (index + len == total) {
              editFile.close();
              request->send(200, "text/plain", "File edited successfully");
          }
      } });
  //===========================End File Manager=================================

  server.onNotFound(notFound);
  server.begin();
}

int32_t getWiFiChannel(const char *ssid)
{
  if (int32_t n = WiFi.scanNetworks())
  {
    for (uint8_t i = 0; i < n; i++)
    {
      if (!strcmp(ssid, WiFi.SSID(i).c_str()))
      {
        return WiFi.channel(i);
      }
    }
  }
  return 0;
}

void reconnect()
{
  while (!client.connected())
  {
    Serial.println("Reconnecting to MQTT broker...");
    if (client.connect(infojson["ME"]["UUID"].as<String>().c_str(),
                       infojson["MQTT"]["DEFAULT_USERNAME"].as<String>().c_str(),
                       infojson["MQTT"]["DEFAULT_PWD"].as<String>().c_str()))
    {
      Serial.println("MQTT broker connected.");
      client.subscribe(infojson["MQTT"]["SUBSCRIBE_TOPIC"][0].as<String>().c_str());
    }
    else
    {
      Serial.print("Failed to reconnect to MQTT broker, rc=");
      Serial.print(client.state());
      Serial.println("Retrying in 5 seconds.");
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200);

  // LittleFS.format();
  if (!LittleFS.begin(true))
  {
#ifdef DEBUG
    Serial.println("An Error has occurred while mounting LITTLEFS");
#endif
  }
  else
  {
    String infojson1;
    service.getDeviceData(infojson1, utilServices, "/info.json");
    DeserializationError err = deserializeJson(infojson, infojson1);
    Serial.println(infojson1);

    user_name = infojson["ME"]["USER_NAME"].as<String>();
    password = infojson["ME"]["PASSWORD"].as<String>();
    mqtt_broker = infojson["MQTT"]["MQTT_SERVER"].as<String>();
    ca_cert = infojson["MQTT"]["SERVER_CERTIFICATE"].as<String>();
  }

  WiFi.mode(WIFI_AP_STA);

  WiFi.softAP(infojson["ME"]["AP_NAME"].as<String>() + infojson["ME"]["UUID"].as<String>());

  WiFi.setSleep(false);

  int32_t channel = getWiFiChannel(infojson["WIFI"]["SSID"].as<String>().c_str());
  Serial.print("WIFI Router Channel: ");
  (channel == 0) ? (Serial.println("0")) : (Serial.println(channel));

  wifiChannel = channel;
  esp_wifi_set_channel(wifiChannel, WIFI_SECOND_CHAN_NONE);

  // setup_wifi();
  autoConnectKnownWifi();

  HandleWeb();

  espClient.setCACert(ca_cert.c_str());
  client.setServer(mqtt_broker.c_str(),
                   infojson["MQTT"]["MQ_PORT"].as<int>());
  client.setCallback(MQTT_CB);
}

void loop()
{
  if (!client.connected())
    reconnect();
  client.loop();

  if (millis() - lastEvent > 1000)
  {
    lastEvent = millis();

    float temperature = random(20, 35);
    float humidity = random(60, 70);
    float pressure = random(80, 95);
    float ldr = random(400, 700) / 10.0;
    int cpu = random(0, 100);

    int ram_total = ESP.getHeapSize();
    int ram_used = ESP.getFreeHeap();

    int psram_total = 0;
    int psram_used = 0;
    int spi_ram_total = 0;

#if CONFIG_SPIRAM_SUPPORT
    if (psramFound())
    {
      psram_total = ESP.getPsramSize();
      psram_used = ESP.getFreePsram();
      spi_ram_total = esp_spiram_get_size();
    }
#endif

    int flash_used = 1024 * (ESP.getFlashChipSize() - ESP.getFreeSketchSpace());
    int flash_total = ESP.getFlashChipSize() / 1024;
    int flash_used_percent = (flash_used * 100) / flash_total;

    String json = "{";
    json += "\"lastEvent\":" + String(lastEvent) + ",";
    json += "\"temperature\":" + String(temperature) + ",";
    json += "\"humidity\":" + String(humidity) + ",";
    json += "\"pressure\":" + String(pressure) + ",";
    json += "\"ldr\":" + String(ldr) + ",";
    json += "\"cpu\":" + String(cpu) + ",";

    json += "\"ram_total\":" + String(ram_total) + ",";
    json += "\"ram_used\":" + String(ram_used) + ",";

    json += "\"psram_total\":" + String(ram_total) + ",";
    json += "\"psram_used\":" + String(ram_used) + ",";

    json += "\"spi_ram_total\":" + String(spi_ram_total) + ",";

    json += "\"flash_total\":" + String(flash_total) + ",";
    json += "\"flash_used\":" + String(flash_used);
    json += "}";

    events.send(json.c_str(), "update", millis());
  }
}