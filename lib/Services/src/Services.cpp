#include "Services.h"

void Services::ConfigFile_Save_Variable(String From, String VarName, String VarValue, UtilServices utilServices)
{
    Serial.print("ConfigFile_Save_Variable: ");
    Serial.print(VarName);
    Serial.print("=");
    Serial.print(VarValue);
    Serial.println();

    File configFile = LittleFS.open("/info.json", "r");
    if (!configFile)
    {
        Serial.println("- failed to open config file for writing");
        return;
    }

    if (configFile.available())
    {
        DynamicJsonDocument doc(2048);
        deserializeJson(doc, configFile);
        utilServices.printLine(" OLD info.json ");
        serializeJsonPretty(doc, Serial);
        configFile.close();

        if (From == "URL")
        {
            doc[VarName].clear();       // Clear the list first,
            doc[VarName].add(VarValue); //  then add to list. To maintain new resiver always first in list.
        }
        else
            doc["ME"][VarName] = VarValue;

        configFile = LittleFS.open("/info.json", "w");
        serializeJson(doc, configFile);
        utilServices.printLine(" NEW info.json ");
        serializeJsonPretty(doc, Serial);
        configFile.close();
        Serial.println("");
        Serial.println(" - config.json saved - OK.");
    }
}

void Services::updateReciverListIfNotFound(String ListName, String mac, String &infojsonAsString, bool EnableDebug, UtilServices utilServices)
{
    DynamicJsonDocument infojson(1024);
    deserializeJson(infojson, infojsonAsString);
    // JsonArray infojson = doc.as<JsonArray>();

    JsonArray remoteList = infojson[ListName].as<JsonArray>();
    int remoteListSize = remoteList.size();
    bool found = false;
    for (JsonVariant v : remoteList)
    {
        if (mac == v.as<String>())
            found = true;
    }
    if (found)
    {
        if (EnableDebug)
            Serial.println(mac + " already found.");
    }
    else
        ConfigFile_Save_Variable("URL", ListName, mac, utilServices);
}

bool Services::getDeviceData(String &infojson, UtilServices utilServices, String filePath = "/info.json")
{
    DynamicJsonDocument infojson1(1024);

    /****** Read info.json *******/
    File info = LittleFS.open(filePath, "r");
    if (!info)
    {
        Serial.println("Failed to open info.json");
        return false;
    }

    String data;
    if (info.available())
        data = info.readString();
    info.close();

    DeserializationError err = deserializeJson(infojson1, data);

    if (err == DeserializationError::Ok)
    {
        utilServices.printLine(" Read JSON Data From File ");
        serializeJsonPretty(infojson1, Serial);
        serializeJsonPretty(infojson1, infojson);
        return true;
    }
    else
    {
        Serial.print("ERROR: deserializeJson() returned.\n");
        Serial.println(err.c_str());
        return false;
    }
    return false;
}
