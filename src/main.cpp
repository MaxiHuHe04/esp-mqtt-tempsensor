#include <Arduino.h>
#include <WiFiManager.h>
#include <settings.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>

#define AP_PASS ""

#define MQTT_HOST ""
#define MQTT_PORT 1883
#define MQTT_USER "esptemp"
#define MQTT_PASSWORD ""

#define OW_PIN D6

#define REQUIRE_WIFI true

void reconnectMQTT();

WiFiManager wifiManager;

OneWire oneWire(OW_PIN);
DallasTemperature sensors(&oneWire);
DeviceAddress tempSensorAddr;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

unsigned long lastMsgMillis = 0;


void setupWifi() {
  loadParameters();

  wifiManager.setConfigPortalTimeout(180);
  wifiManager.setBreakAfterConfig(true);  // Save config even when connection was unsuccessful
  wifiManager.setConnectRetries(2);
  wifiManager.setSaveConfigCallback(saveParameters);
  wifiManager.setWiFiAutoReconnect(true);

  for (int i = 0; i < wifiManagerParamsCount; i++) {
    wifiManager.addParameter(wifiManagerParams[i]);
  }

  #if !REQUIRE_WIFI
  wifiManager.setConfigPortalBlocking(false);
  #endif

  byte macAddr[6];
  WiFi.macAddress(macAddr);
  char apSSID[21] = "ESPTemp-";
  char* apSSIDHexPtr = apSSID + 8;
  for (int i = 0; i < 6; i++) {
    snprintf(apSSIDHexPtr, 3, "%X", macAddr[i]);
    apSSIDHexPtr += 2;
  }
  apSSID[20] = 0;

  bool connected = wifiManager.autoConnect(apSSID, AP_PASS);

  if (!connected) {
    Serial.println("Failed to connect to WiFi.");

    #if REQUIRE_WIFI
    delay(2000);
    ESP.restart();
    #else
    wifiManager.setConfigPortalTimeout(0);
    #endif
  }

  wifiManager.setConfigPortalBlocking(false);
  wifiManager.startConfigPortal(apSSID, AP_PASS);  // Start config portal also when connected to change settings

  wifiManager.setConfigPortalTimeoutCallback([]() {
    wifiManager.startWebPortal();
  });
}

void setupArduinoOTA() {
  ArduinoOTA.onStart([]() {
    Serial.println("Start updating");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress * 100 / total));
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });

  ArduinoOTA.begin();

  Serial.printf("OTA hostname: %s", ArduinoOTA.getHostname().c_str());
  Serial.println();
}


void setup() {
  WiFi.mode(WIFI_STA);
  Serial.begin(115200);
  beginEEPROM();
  
  Serial.println();
  
  setupWifi();
  setupArduinoOTA();

  sensors.begin();
  
  if (!sensors.getAddress(tempSensorAddr, 0)) {
    Serial.println("Unable to find address for device 0");
  }

  mqttClient.setServer(MQTT_HOST, 1883);

  sensors.setResolution(tempSensorAddr, 9);
}

void loop() {
  wifiManager.process();
  if (!wifiManager.getConfigPortalActive()) {
    if (settingsChanged || (REQUIRE_WIFI && WiFi.status() != WL_CONNECTED)) {
      ESP.restart();
    }
  }

  ArduinoOTA.handle();

  long now = millis();
  if (now - lastMsgMillis > 10000){    
    lastMsgMillis = now;
    sensors.requestTemperatures();
    float tempC = sensors.getTempC(tempSensorAddr);
    if (!mqttClient.connected()) {
      reconnectMQTT();
    }
    mqttClient.publish(String(mqttTopicName + "/temperature").c_str(), String(tempC).c_str(), false);
  }
  mqttClient.loop();
}

void reconnectMQTT() {
  while (!mqttClient.connected()){
    Serial.println("Attempting MQTT connection.....");
    if (mqttClient.connect("ESP8266Client", MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("connected");
    } else{
      Serial.printf("Failed, rc=%d", mqttClient.state());
      Serial.println();
      delay(5000);
    }
  }
}