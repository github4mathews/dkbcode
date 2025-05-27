#include "UtilServices.h"

void UtilServices::printLine(String message)
{
    Serial.println();
    printDash(10);

    Serial.print(message);

    printDash(10);
    Serial.println();
}

void UtilServices::printDash(int count)
{
    for (size_t i = 0; i < (size_t)count; i++)
        Serial.print("=");
}

void UtilServices::printInitializingDebug(String reciverListAsString,
                                          bool peerAdded,
                                          uint8_t broadcastAddress[6],
                                          int WorkingMode)
{
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, reciverListAsString);
    JsonArray reciverList = doc.as<JsonArray>();

    printLine(" START Initializing Debug ");
    Serial.println();
    Serial.print("No of Reciver List in JSON: ");
    Serial.println(reciverList.size());

    if (reciverList.size() > 0 && peerAdded == 1)
        Serial.println("All peer adaded from info.json");
    else if (reciverList.size() == 0 && peerAdded == 1)
        Serial.println("Broadcast address added to peer list.");

    // uint8_t all_cont[1];
    // uint8_t encrypt_cont[1];
    // esp_now_get_cnt_info(all_cont, encrypt_cont);
    // Serial.print("Peer Count: ");
    // Serial.println(all_cont[1]);
    Serial.print("Reciver Address: ");
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
             broadcastAddress[0], broadcastAddress[1], broadcastAddress[2], broadcastAddress[3], broadcastAddress[4], broadcastAddress[5]);
    Serial.println(macStr);
    Serial.print("Working Mode: ");
    Serial.println(WorkingMode);
    printLine(" END Initializing Debug ");
}

void UtilServices::printOutgoingMessage(String outgoingJSON)
{
    DynamicJsonDocument outgoingJSON_(1024);
    deserializeJson(outgoingJSON_, outgoingJSON);

    printLine(" OGM JSON Data ");
    Serial.println("OGM.From:\t" + outgoingJSON_["From"].as<String>());
    Serial.println("OGM.To:\t" + outgoingJSON_["To"].as<String>());
    Serial.println("OGM.Header:\t" + outgoingJSON_["Header"].as<String>());
    Serial.println("OGM.Msg:\t" + outgoingJSON_["Msg"].as<String>());
    printLine(" END OGM JSON Data ");
}