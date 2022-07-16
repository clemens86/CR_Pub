/**
 * PN5180 RFID reading
 * Reads RFID Tags and sends their UID via MQTT
 * Works for 14443 Tags.
 */

// DEFINES
#define devicename "RFID Alien ESP"
// #define DEBUG 
//MQTT Pub Topics
#define MQTT_PUB_PRES "esp/presence"
#define MQTT_PUB_UID "esp/AlienRFID/UID"
//MQTT Sub Topics
#define MQTT_SUB_RES "esp/reset/AlienRFID"

// INCLUDES
#include "Arduino.h"
// Download from https://github.com/playfultechnology/PN5180-Library
#include <PN5180.h>
#include <PN5180ISO15693.h>
#include "WiFi.h"
#include <AsyncMqttClient.h>
#include "AsyncTCP.h"
#include "ESPAsyncWebServer.h"
#include "AsyncElegantOTA.h"
#include "configheader.h" 


// ssid/password, mqtt server/pwd/username, hostname are in configheader.h 

char state;

// CONSTANTS
// The number of PN5180 readers connected
const byte numReaders = 1;

// GLOBALS
// Each PN5180 reader requires unique NSS, BUSY, and RESET pins,
// as defined in the constructor below
#define PN5180_NSS  5
#define PN5180_BUSY 16
#define PN5180_RST  17
#include <PN5180ISO14443.h>
//If using ISO14443, uncommment below
PN5180ISO14443 nfc14443(PN5180_NSS, PN5180_BUSY, PN5180_RST); 

//If using ISO15693, uncommment below
//PN5180ISO15693 nfc(PN5180_NSS, PN5180_BUSY, PN5180_RST); 

// Array to record the value of the last UID read by each reader
uint8_t thisUid[4];
uint8_t lastUid[4];

AsyncWebServer server(80);

WiFiClient wifiClient;

AsyncMqttClient mqttClient;

unsigned long previousMillis = 0;   // Stores last time temperature was published
const long interval = 10000;        // Interval at which to publish sensor readings

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void WiFiEvent(WiFiEvent_t event) {
    Serial.printf("[WiFi-event] event: %d\n", event);
    switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
        connectToMqtt();
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        Serial.println("WiFi lost connection");
        break;
    }
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
  mqttClient.publish(MQTT_PUB_PRES, 1, true, devicename " just came online:");
  mqttClient.publish(MQTT_PUB_PRES, 1, true, "will publish UID here: " MQTT_PUB_UID);                         
  Serial.printf("published Message: ", devicename "on" MQTT_PUB_PRES);
  mqttClient.subscribe(MQTT_SUB_RES, 1);
  Serial.println("Subscribed to " MQTT_SUB_RES);
  String ipaddress = WiFi.localIP().toString();
  char ipchar[ipaddress.length()+1];
  ipaddress.toCharArray(ipchar,ipaddress.length()+1);
  mqttClient.publish(MQTT_PUB_PRES, 1, true, "my IP is: ");
  mqttClient.publish(MQTT_PUB_PRES, 1, true, ipchar);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected()) {
    connectToMqtt();
  }
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}

void onMqttUnsubscribe(uint16_t packetId) {
  Serial.println("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  Serial.println("Publish received.");
  Serial.print("  topic: ");
  Serial.println(topic);
  Serial.print("  payload: ");
  for (int i = 0; i < len; i++) {
    Serial.print((char)payload[i]);
  }
  if ((char)payload[0] == '1') {
    Serial.println("Restarting ESP.");
    ESP.restart();
  }
}

void onMqttPublish(uint16_t packetId) {
  Serial.print("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

byte nibble(byte val) {
  val &= 0xF;
  return val+(val<10 ? '0' : 'A'-10);
}
void shex(char* store, byte* from, byte len) {
  while (len--) {
    *store++ = nibble(*from>>4);
    *store++ = nibble(*from++);
  }
  *store = 0;
}

void setup() {
  // Initialise serial connection
  Serial.begin(115200);  
  WiFi.onEvent(WiFiEvent);
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  //mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  // If your broker requires authentication (username and password), set them below
  //mqttClient.setCredentials("REPlACE_WITH_YOUR_USER", "REPLACE_WITH_YOUR_PASSWORD");
  
  connectToWifi();
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! I am ESP32, use IP/update for OTA Update.");
  });
  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();
  Serial.println("HTTP server started");  

  // Initialise Reader
    Serial.println(F("Initialising Reader..."));
    nfc14443.begin();
    Serial.println(F("Resetting Reader..."));
    nfc14443.reset();    
    Serial.println(F("Enabling RF field..."));
    nfc14443.setupRF();
    Serial.println(F("Reader Setup Complete"));  
}

void loop() {
  unsigned long currentMillis = millis();
  Serial.println(F("Starting loop"));
  nfc14443.reset();
  nfc14443.setupRF();
  uint8_t thisUid[4] = {0x00,0x00,0x00,0x00};
  uint8_t clearUid[4] = {0x00,0x00,0x00,0x00};
  uint8_t uidLength = nfc14443.readCardSerial(thisUid);
  if(memcmp(thisUid, clearUid, 4) != 0) {
    Serial.println(F("thisUid is not 0"));
    Serial.println();      
    Serial.println(F("publishing UID to MQTT"));
    char pubUid[9];
    shex(pubUid, thisUid, sizeof(thisUid));
    mqttClient.publish(MQTT_PUB_UID, 1, true, pubUid);
    delay(1500);
    Serial.println();    
    if (uidLength > 0) {
      Serial.print(F("ISO-14443 card found, UID="));
      for (int i=0; i<uidLength; i++) {
        Serial.print(thisUid[i] < 0x10 ? " 0" : " ");
        Serial.print(thisUid[i], HEX);
       }
        Serial.println(F("----------------------------------"));
        if(memcmp(thisUid, lastUid, 4) == 0) {
          Serial.println(F("thisUid and lastUid is equal"));
        }
        else {
          Serial.println(F("different card on reader"));
        }
        memcpy(lastUid, thisUid, 4);
  }
  } 
  else {
      //Init clear UID Pub
  char pubClearUid[9];
  shex(pubClearUid, clearUid, sizeof(thisUid));
  mqttClient.publish(MQTT_PUB_UID, 1, true, pubClearUid);
    }
    Serial.println(F("Ending loop"));
    }