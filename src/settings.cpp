#include <settings.h>
#include <EEPROM.h>
#include <WiFiManager.h>

#define EEPROM_SIZE 512  // Usable is only half of this

#define EEPROM_TOPIC_START 7
#define EEPROM_TOPIC_MAX_LENGTH 128
#define EEPROM_TOPIC_DEFAULT "livingroom"

const char eepromCheckData[] = "TMPSENS";
const int eepromCheckLength = strlen(eepromCheckData);

WiFiManagerParameter mqttTopicNameParam("topicname", "MQTT topic names (sensor/&lt;name&gt;/temperature, comma separated)",
  EEPROM_TOPIC_DEFAULT, EEPROM_TOPIC_MAX_LENGTH);

WiFiManagerParameter* wifiManagerParams[] = {&mqttTopicNameParam};
const word wifiManagerParamsCount = sizeof(wifiManagerParams) / sizeof(wifiManagerParams[0]);

char mqttTopicNameString[EEPROM_TOPIC_MAX_LENGTH] = EEPROM_TOPIC_DEFAULT;
String mqttTopicNames[MAX_SENSOR_COUNT] = {};
int sensorCount = 0;

bool settingsChanged = false;

void writeEEPROM(int location, byte value) {
  EEPROM.write(location, value);
  if (EEPROM_SIZE / 2 > location) EEPROM.write(EEPROM_SIZE - 1 - location, value);  // write twice for integrity checking
  else Serial.println("EEPROM size not large enough for double write"); 
}

void writeEEPROM(int location, String value) {
  for (unsigned int i = 0; i < value.length(); i++) {
    writeEEPROM(location + i, value.charAt(i));
  }
  writeEEPROM(location + value.length(), '\0');
}

void readEEPROMString(char* buffer, int location, int maxLength) {
  for (int i = 0; i < maxLength; i++) {
    buffer[i] = EEPROM.read(location + i);
    if (buffer[i] == '\0') {
      break;
    }
  }
}

void resetEEPROM() {
  for (int i = 0; i < eepromCheckLength; i++) {
    writeEEPROM(i, eepromCheckData[i]);
  }
  for (int i = eepromCheckLength; i < EEPROM_SIZE / 2; i++) {
    writeEEPROM(i, 0);
  }

  saveParameters();
}

bool checkEEPROM() {
  for (int i = 0; i < eepromCheckLength; i++) {
    if (EEPROM.read(i) != eepromCheckData[i]) return false;
  }

  for (int i = 0; i < EEPROM_SIZE / 2; i++) {
    if (EEPROM.read(i) != EEPROM.read(EEPROM_SIZE - 1 - i)) {
      Serial.printf("EEPROM value %d at %d doesn't match %d at %d", EEPROM.read(i), i, EEPROM.read(EEPROM_SIZE - 1 - i), EEPROM_SIZE - 1 - i);
      Serial.println();
      return false;
    }
  }
  
  return true;
}

void beginEEPROM() {
  EEPROM.begin(EEPROM_SIZE);
  if (!checkEEPROM()) {
    Serial.println("EEPROM invalid. Initializing");
    resetEEPROM();
  }
}

void splitTopicNames() {
  char tokenString[EEPROM_TOPIC_MAX_LENGTH];
  strncpy(tokenString, mqttTopicNameString, EEPROM_TOPIC_MAX_LENGTH);

  char* rest = nullptr;
  char* token = strtok_r(tokenString, ",", &rest);
  int i = 0;

  while (token != nullptr) {
    mqttTopicNames[i] = token;

    token = strtok_r(nullptr, ",", &rest);
    i++;
  }

  sensorCount = i;
}

void saveParameters() {
  strncpy(mqttTopicNameString, mqttTopicNameParam.getValue(), EEPROM_TOPIC_MAX_LENGTH);
  splitTopicNames();

  Serial.printf("Saving parameters: MQTT topic name: %s", mqttTopicNameString);
  Serial.println();

  writeEEPROM(EEPROM_TOPIC_START, mqttTopicNameString);

  if (!EEPROM.commit()) {
    Serial.println("EEPROM commit failed");
  }

  settingsChanged = true;
}

void loadParameters() {
  readEEPROMString(mqttTopicNameString, EEPROM_TOPIC_START, EEPROM_TOPIC_MAX_LENGTH);
  splitTopicNames();

  Serial.printf("Loaded parameters: MQTT topic names: %s, %d sensors", mqttTopicNameString, sensorCount);
  Serial.println();

  // Lengths set the maximum possible, but null terminators before also work (strncpy)
  mqttTopicNameParam.setValue(mqttTopicNameString, mqttTopicNameParam.getValueLength());
}

void printEEPROM() {
  for (int i = 0; i < EEPROM_SIZE; i++) {
    Serial.print(EEPROM.read(i), HEX);
    Serial.print(" ");
  }
  Serial.println();
}
