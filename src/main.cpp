#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
//W
// Define team ID
String TeamID;
// BLE server name
#define bleServerName "ESP_BLE A27"
// Define variables for the connection status
bool connected = false;
bool connected2 = false;
DynamicJsonDocument JsonDoc(4096);
// Generate a unique UUID for your Bluetooth service
// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
#define SERVICE_UUID "c7d84d70-dac8-11ed-afa1-0242ac120002"
#define CHARACTERISTIC_UUID "d1815718-dac8-11ed-afa1-0242ac120002"
// Define a caracteristic with the properties:
// Read, Write (with response), Notify
// Use the above link for generating UUIDs
BLEServer *pServer;
BLECharacteristic characteristic(CHARACTERISTIC_UUID, 
BLECharacteristic::PROPERTY_READ | 
BLECharacteristic::PROPERTY_WRITE | 
BLECharacteristic::PROPERTY_NOTIFY);

// Define a descriptor characteristic
// IMPORTANT -- The characteristic should have the descriptor UUID
// 0x2902 or 0x2901
BLEDescriptor *characteristicDescriptor = new BLEDescriptor(BLEUUID((uint16_t)0x2902));
// Setup callbacks onConnect and onDisconnect
class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    connected = true;
    Serial.println("Device connected");
  }
  void onDisconnect(BLEServer* pServer) {
    connected = false;
    Serial.println("Device disconnected");
  }
};
// Setup callbacks for Charactersictic
class MyCharacteristicCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic){
    std::string tmpCharacteristic =  pCharacteristic->getValue();
    Serial.println(tmpCharacteristic.c_str());
    deserializeJson(JsonDoc,tmpCharacteristic.c_str());
    std::string jsonaction = JsonDoc["action"];
    //Setup for network scan
    if(jsonaction.compare("getNetworks") == 0){
      
      std::string teamID = JsonDoc["teamId"];
      //Scan all networks, serialize values and print json objects
      int n = WiFi.scanNetworks();
      
      for(int i = 0 ; i < n ; ++i){
        DynamicJsonDocument networkDoc(4096);
        JsonObject jsonnetwork = networkDoc.to<JsonObject>();
        
        jsonnetwork["ssid"] = WiFi.SSID(i);
        jsonnetwork["strength"] = WiFi.RSSI(i);
        jsonnetwork["encryption"] = WiFi.encryptionType(i);
        jsonnetwork["teamId"] = teamID;
        
        TeamID = String(teamID.c_str());
        std::string jsonData;
        serializeJson(networkDoc,jsonData);
        serializeJson(networkDoc,Serial);
        pCharacteristic->setValue(jsonData);
        pCharacteristic->notify();
      }
    }
    //Connect to network
    
    else if(jsonaction.compare("connect") == 0){
      bool temp_connected;
      std::string jsonssid = JsonDoc["ssid"];
      std::string jsonpassword = JsonDoc["password"];
      WiFi.begin(jsonssid.c_str(), jsonpassword.c_str());
      delay(5000);
      if(WiFi.status() == WL_CONNECTED){
          temp_connected = true;
          Serial.println("Connection succed");
      }
      else{
          temp_connected = false;
          Serial.println("Connection failed");
      }
      DynamicJsonDocument connectDoc(4096);
          JsonObject jsonconnect = connectDoc.to<JsonObject>();

          jsonconnect["ssid"] = WiFi.SSID();
          jsonconnect["connected"] = temp_connected;
          jsonconnect["teamId"] = TeamID;
          std::string jsonData;
          serializeJson(connectDoc,jsonData);
          serializeJson(connectDoc,Serial);
          pCharacteristic->setValue(jsonData);
          pCharacteristic->notify(); 
    }
    else if(jsonaction.compare("getData") == 0){ //deserialize data after request
        DynamicJsonDocument getDataDoc(4096);
        String URL = "http://proiectia.bogdanflorea.ro/api/disney-characters/characters";
        HTTPClient http;
        http.begin(URL);
        if(http.GET() == 200){
          String payload = http.getString();
          deserializeJson(getDataDoc, payload);
          JsonArray vec = getDataDoc.as<JsonArray>();
          for(JsonObject x : vec){
              if(x.containsKey("_id")){
                x["id"] = x["_id"];
                x.remove("_id");
              }
              if(x.containsKey("imageUrl")){
                x["image"] = x["imageUrl"];
                x.remove("imageUrl");
              }
              
              x["teamId"] = TeamID;
              DynamicJsonDocument setDataDoc(4096);
              setDataDoc.set(x);
              std::string jsonData;
              serializeJson(setDataDoc, jsonData);
              pCharacteristic->setValue(jsonData);
              pCharacteristic->notify();
          }
        }
        http.end();
      }
      else if(jsonaction.compare("getDetails") == 0){  //deserialize details after request
            DynamicJsonDocument recieveDoc(16384);
            DynamicJsonDocument Doc(16384);
            std::string ID = JsonDoc["id"];
            String temp_URL = "http://proiectia.bogdanflorea.ro/api/disney-characters/character?_id=";
            String URL = temp_URL + String(ID);
            Serial.println(URL.c_str());
            HTTPClient http;
            http.begin(URL);
            if(http.GET() == 200){
                String payload = http.getString();
                deserializeJson(recieveDoc, payload);
                JsonObject object = Doc.to<JsonObject>();
                
                String description;

                JsonArray films = recieveDoc["films"];
                if(films.size() > 0){
                    description += "Films = ";
                    for(int i = 0 ; i < films.size() ; ++i)
                        description += (films[i].as<String>() + ", ");
                    description += "\n";
                }

                JsonArray shortFilms = recieveDoc["shortFilms"];
                if(shortFilms.size() > 0){
                    description += "shortFilms = ";
                    for(int i = 0 ; i < shortFilms.size() ; ++i)
                        description += (shortFilms[i].as<String>() + ", ");
                    description += "\n";
                }

                JsonArray tvShows = recieveDoc["tvShows"];
                if(tvShows.size() > 0){
                    description += "tvShows = ";
                    for(int i = 0 ; i < tvShows.size() ; ++i)
                        description += (tvShows[i].as<String>() + ", ");
                    description += "\n";
                }

                JsonArray videoGames = recieveDoc["videoGames"];
                if(videoGames.size() > 0){
                    description += "videoGames = ";
                    for(int i = 0 ; i < videoGames.size() ; ++i)
                        description += (videoGames[i].as<String>() + ", ");
                    description += "\n";
                }

                JsonArray parkAttractions = recieveDoc["parkAttractions"];
                if(parkAttractions.size() > 0){
                    description += "parkAttractions = ";
                    for(int i = 0 ; i < parkAttractions.size() ; ++i)
                        description += (parkAttractions[i].as<String>() + ", ");
                    description += "\n";
                }

                JsonArray allies = recieveDoc["allies"];
                if(allies.size() > 0){
                    description += "allies = ";
                    for(int i = 0 ; i < allies.size() ; ++i)
                        description += (allies[i].as<String>() + ", ");
                    description += "\n";
                }

                JsonArray enemies = recieveDoc["enemies"];
                if(enemies.size() > 0){
                    description += "enemies = ";
                    for(int i = 0 ; i < enemies.size() ; ++i)
                        description += (enemies[i].as<String>() + ", ");
                    description += "\n";
                }

                object["description"] = description;
                
                String id = recieveDoc["_id"];
                object["id"] = id;
                String name = recieveDoc["name"];
                object["name"] = name; 
                String image = recieveDoc["imageUrl"];
                object["image"] = image;
                String url = recieveDoc["url"];
                object["url"] = url;
                object["teamId"] = TeamID;

                std::string jsonData;
                serializeJson(Doc, jsonData);
                pCharacteristic->setValue(jsonData);
                pCharacteristic->notify();
            }
            http.end();
      }
    }
 };
void setup() {
 // Start serial communication
 Serial.begin(115200);
 // Create the BLE Device
 BLEDevice::init(bleServerName);
 // Create the BLE Server
 BLEServer *pServer = BLEDevice::createServer();
 // Set server callbacks
 pServer->setCallbacks(new MyServerCallbacks());

 //Create characteristic callbacks
 characteristic.setCallbacks(new MyCharacteristicCallbacks());
 // Create the BLE Service
 BLEService *bleService = pServer->createService(SERVICE_UUID);

 bleService->addCharacteristic(&characteristic);
 characteristic.addDescriptor(characteristicDescriptor);

 // Start the service
 bleService->start();
 // Start advertising
 BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
 pAdvertising->addServiceUUID(SERVICE_UUID);
 pServer->getAdvertising()->start();
 Serial.println("Waiting a client connection to notify...");

 WiFi.mode(WIFI_STA);
 WiFi.disconnect();
 delay(3000);
 
}
void loop() {

}
