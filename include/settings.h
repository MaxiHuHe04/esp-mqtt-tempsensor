#include <Arduino.h>
#include <WiFiManager.h>

#define MAX_SENSOR_COUNT 5

void beginEEPROM();
void writeEEPROM(int address, byte value);
void writeEEPROM(int address, String value);
void resetEEPROM();
bool checkEEPROM();
void readEEPROMString(char* buffer, int location, int maxLength);

void saveParameters();
void loadParameters();

void printEEPROM();

extern String mqttTopicNames[MAX_SENSOR_COUNT];
extern int sensorCount;

extern bool settingsChanged;

extern WiFiManagerParameter* wifiManagerParams[];
extern const word wifiManagerParamsCount;
