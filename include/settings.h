#include <Arduino.h>
#include <WiFiManager.h>

void beginEEPROM();
void writeEEPROM(int address, byte value);
void writeEEPROM(int address, String value);
void resetEEPROM();
bool checkEEPROM();
String readEEPROMString(int location, int maxLength);

void saveParameters();
void loadParameters();

void printEEPROM();

extern String mqttTopicName;

extern bool settingsChanged;

extern WiFiManagerParameter* wifiManagerParams[];
extern const word wifiManagerParamsCount;
