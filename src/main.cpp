#define ARDUINOJSON_ENABLE_COMMENTS 1
bool EnableDebug = true;

#include <Arduino.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <String.h>
#include <FS.h>
#include <spiffsServices.h>
#include <utlityServices.h>
#include <StringHandlingServices.h>
#include <iostream>
#include <SoftwareSerial.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#include <Update.h>
#include <esp32/spiram.h>

using namespace std;

int wifiChannel = 10;

WiFiClientSecure espClient;
PubSubClient client(espClient);

AsyncWebServer server(80);
AsyncEventSource events("/events");
unsigned long lastEvent = 0;

SpiffsServices spiffsService = SpiffsServices();
UtlityServices utlityService = UtlityServices();
StringHandlingServices stringHandlingService = StringHandlingServices();

JsonDocument infojson;

String mqtt_broker;
String ca_cert;
String user_name;
String password;

// ==========================  WEB HANDLING    ===================================
const char *BSSID = "bssid";
const char *PASS = "password";
const char *SENSOR = "sensor";

void notFound(AsyncWebServerRequest *request);
void HandleWeb();
String getParameter(AsyncWebServerRequest *request);
int32_t getWiFiChannel(const char *ssid);
void setup_wifi();
void reconnect();
void MQTT_CB(char *topic, byte *payload, unsigned int length);
void printLine(String str);

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
  File file = SPIFFS.open("/info.json", "r");
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
  File file = SPIFFS.open("/info.json", "w");
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
            { request->send(SPIFFS, "/index.html", "text/html"); });
  server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/index.html", "text/html"); });
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/favicon.ico", "image/x-icon"); });
  server.on("/login.html", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/login.html", "text/html"); });
  server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/styles.css", "text/css"); });
  // Serve JS
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/script.js", "text/javascript"); });
  server.on("/logo.svg", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/logo.svg", "image/svg+xml"); });
  server.on("/profile.png", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/profile.png", "image/png"); });

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
          DynamicJsonDocument doc(8192);
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
      DynamicJsonDocument doc(8192);
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
          DynamicJsonDocument doc(8192);
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
          DynamicJsonDocument doc(8192);
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

void setup_wifi()
{
  WiFi.begin(infojson["WIFI"]["SSID"].as<String>().c_str(),
             infojson["WIFI"]["PWD"].as<String>().c_str());

  Serial.println("Connecting to WiFi");
  Serial.print("SSID: ");
  Serial.println(infojson["WIFI"]["SSID"].as<String>());
  Serial.print("PWD: ");
  Serial.println(infojson["WIFI"]["PWD"].as<String>());

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    randomSeed(micros());
    Serial.println((String) "\nWiFi connected\nIP address: " + WiFi.localIP());

    Serial.print("Station IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Wi-Fi Channel: ");
    Serial.println(WiFi.channel());
  }
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
  Serial.begin(9600);

  if (!SPIFFS.begin())
  {
    if (EnableDebug)
      Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  else
  {
    String infojson1;
    spiffsService.getDeviceData(infojson1, utlityService, "/info.json");
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

  setup_wifi();

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