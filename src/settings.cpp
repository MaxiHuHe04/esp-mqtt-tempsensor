#include <settings.h>
#include <EEPROM.h>
#include <WiFiManager.h>

#define EEPROM_SIZE 128  // Usable is only half of this

#define EEPROM_TOPIC_START 7
#define EEPROM_TOPIC_MAX_LENGTH 32
#define EEPROM_TOPIC_DEFAULT "livingroom"

const char eepromCheckData[] = "TMPSENS";
const int eepromCheckLength = strlen(eepromCheckData);

WiFiManagerParameter mqttTopicNameParam("topicname", "MQTT topic name (<name>/temperature)",
  EEPROM_TOPIC_DEFAULT, EEPROM_TOPIC_MAX_LENGTH);

WiFiManagerParameter* wifiManagerParams[] = {&mqttTopicNameParam};
const word wifiManagerParamsCount = sizeof(wifiManagerParams) / sizeof(wifiManagerParams[0]);

String mqttTopicName = EEPROM_TOPIC_DEFAULT;

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

String readEEPROMString(int location, int maxLength) {
  char buffer[maxLength];
  for (int i = 0; i < maxLength; i++) {
    buffer[i] = EEPROM.read(location + i);
    if (buffer[i] == '\0') {
      break;
    }
  }
  
  return buffer;
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

void saveParameters() {
  mqttTopicName = mqttTopicNameParam.getValue();

  Serial.printf("Saving parameters: MQTT topic name: %s", mqttTopicName.c_str());
  Serial.println();

  writeEEPROM(EEPROM_TOPIC_START, mqttTopicName);

  if (!EEPROM.commit()) {
    Serial.println("EEPROM commit failed");
  }

  settingsChanged = true;
}

void loadParameters() {
  mqttTopicName = readEEPROMString(EEPROM_TOPIC_START, EEPROM_TOPIC_MAX_LENGTH);

  Serial.printf("Loaded parameters: MQTT topic name: %s", mqttTopicName.c_str());
  Serial.println();

  // Lengths set the maximum possible, but null terminators before also work (strncpy)
  mqttTopicNameParam.setValue(mqttTopicName.c_str(), mqttTopicNameParam.getValueLength());
}

void printEEPROM() {
  for (int i = 0; i < EEPROM_SIZE; i++) {
    Serial.print(EEPROM.read(i), HEX);
    Serial.print(" ");
  }
  Serial.println();
}
