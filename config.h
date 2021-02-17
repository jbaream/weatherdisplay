#ifndef CONFIG_H
#define CONFIG_H

#include <EEPROM.h>

#define EEPROM_SIZE 128

String CONFIG[] = {
  "yourSSID",
  "yourPasswsord",
  "yourApiKey",
  "api.openweathermap.org",
  "Default",
  "RU",
  "RU",
  "north",
  "M",
  "MSK-3",
  "0.ru.pool.ntp.org"
};

String getSsid() {
  return CONFIG[0];
}

String getPassword() {
  return CONFIG[1];
}

// API key from https://openweathermap.org/
String getApiKey() {
  return CONFIG[2];
}
// "api.openweathermap.org"
String getServer() {
  return CONFIG[3];
}

String getCity() {
  return CONFIG[4];
}

String getCountry() {
  return CONFIG[5];
}

String getLanguage() {
  return CONFIG[6];
}

String getHemisphere() {
  return CONFIG[7];
}

String getUnits() {
  return CONFIG[8];
}

String getTimezone() {
  return CONFIG[9];
}

String getNtpServer() {
  return CONFIG[10];
}

void readConfigFromEEPROM() {
  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("ERROR: Failed to initialise EEPROM");
    return;
  }
  Serial.print("INFO: Reading config from EEPROM... ");
  for (int i = 0, a = 0; i < 10; i++) {
    uint8_t len = EEPROM.readByte(a++);
    char str[len];
    for (int j = 0; j < len; j++)
      str[j] = EEPROM.readChar(a++);
    CONFIG[i] = String(str);
  }
  Serial.println("DONE");
}

void storeConfigToEEPROM() {
  if (!EEPROM.begin(200)) {
    Serial.println("ERROR: Failed to initialise EEPROM");
    return;
  }
  Serial.print("INFO: Storing config to EEPROM... ");
  for (int i = 0, a = 0; i < 10; i++) {
    uint8_t len = CONFIG[i].length() + 1;
    char str[len];
    CONFIG[i].toCharArray(str, len);
    EEPROM.writeByte(a++, len);
    for (int j = 0; j < len; j++)
      EEPROM.writeChar(a++, str[j]);
  }
  EEPROM.commit();
  Serial.println("DONE");
}

void readConfigFromFile(File file) {
  if (!file) {
    Serial.println("ERROR: Failed to open WI-Fi config file");
    return;
  }
  Serial.print("Reading config from file... ");
  DynamicJsonDocument config(2048);
  deserializeJson(config, file);
  JsonObject object = config.as<JsonObject>();
  CONFIG[0]   = object["ssid"].as<String>();
  CONFIG[1]   = object["password"].as<String>();
  CONFIG[2]   = object["apikey"].as<String>();
  CONFIG[3]   = object["server"].as<String>();
  CONFIG[4]   = object["City"].as<String>();
  CONFIG[5]   = object["Country"].as<String>();
  CONFIG[6]   = object["Language"].as<String>();
  CONFIG[7]   = object["Hemisphere"].as<String>();
  CONFIG[8]   = object["Units"].as<String>();
  CONFIG[9]   = object["Timezone"].as<String>();
  CONFIG[10]  = object["ntpServer"].as<String>();
  Serial.println("DONE");
  storeConfigToEEPROM();
  file.close();
}

#endif
