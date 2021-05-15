#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <SPI.h>
#include <SD.h>

#define CONFIG_FILE_NAME "/wifi_config.txt"

#define SDCARD_SS     13
#define SDCARD_CLK    14
#define SDCARD_MOSI   15
#define SDCARD_MISO   2

#define EEPROM_SIZE   200

namespace Config
{
  enum EepromId
  {
    SSID,
    PASSWORD,
    API_KEY,
    API_URL,
    CITY,
    LATITUDE,
    LONGITUDE,
    COUNTRY,
    LANG,
    HEMISPHERE,
    UNITS,
    TIMEZONE,
    NTP_URL,
    _LAST_ID_
  };

  void dump_confg();

  String CONFIG[] =
  {
      "yourSSID",
      "yourPasswsord",
      "yourApiKey",
      "api.openweathermap.org",
      "City",
      "latitude",
      "longitude",
      "RU",
      "RU",
      "north",
      "M",
      "MSK-3",
      "0.ru.pool.ntp.org"
  };

  String getSsid()
  {
    return CONFIG[SSID];
  }

  String getPassword()
  {
    return CONFIG[PASSWORD];
  }

  // API key from https://openweathermap.org/
  String getApiKey()
  {
    return CONFIG[API_KEY];
  }
  // "api.openweathermap.org"
  String getServer()
  {
    return CONFIG[API_URL];
  }

  String getCity()
  {
    return CONFIG[CITY];
  }

  String getLatitude()
  {
    return CONFIG[LATITUDE];
  }

  String getLongitude()
  {
    return CONFIG[LONGITUDE];
  }

  String getCountry()
  {
    return CONFIG[COUNTRY];
  }

  String getLanguage()
  {
    return CONFIG[LANG];
  }

  String getHemisphere()
  {
    return CONFIG[HEMISPHERE];
  }

  String getUnits()
  {
    return CONFIG[UNITS];
  }

  boolean isMetric()
  {
    return getUnits() == "M";
  }

  String getTimezone()
  {
    return CONFIG[TIMEZONE];
  }

  String getNtpServer()
  {
    return CONFIG[NTP_URL];
  }

  void read_eeprom()
  {
    if (!EEPROM.begin(EEPROM_SIZE))
    {
      Serial.println("ERROR: Failed to initialise EEPROM");
      return;
    }
    Serial.print("INFO: Reading config from EEPROM... ");
    for (int i = 0, a = 0; i < _LAST_ID_; i++)
    {
      uint8_t len = EEPROM.readByte(a++);
      char str[len];
      for (int j = 0; j < len; j++)
        str[j] = EEPROM.readChar(a++);
      CONFIG[i] = String(str);
    }
    Serial.println("DONE");
  }

  void store_eeprom()
  {
    if (!EEPROM.begin(EEPROM_SIZE))
    {
      Serial.println("ERROR: Failed to initialise EEPROM");
      return;
    }
    Serial.print("INFO: Storing config to EEPROM... ");
    for (int i = 0, a = 0; i < _LAST_ID_; i++)
    {
      uint8_t data_length = CONFIG[i].length() + 1;
      char buffer[data_length];
      CONFIG[i].toCharArray(buffer, data_length);
      EEPROM.writeByte(a++, data_length);
      for (int b = 0; b < data_length; b++)
      {
        EEPROM.writeByte(a++, buffer[b]);
      }
      
    }
    EEPROM.commit();
    Serial.println("DONE");
  }

  void read_sd_card(File file)
  {
    if (!file)
    {
      Serial.println("ERROR: Failed to open config file on SD card");
      return;
    }
    Serial.print("INFO: Reading config from SD card... ");
    DynamicJsonDocument config(2048);
    deserializeJson(config, file);
    JsonObject object = config.as<JsonObject>();
    CONFIG[SSID] = object["ssid"].as<String>();
    CONFIG[PASSWORD] = object["password"].as<String>();
    CONFIG[API_KEY] = object["apikey"].as<String>();
    CONFIG[API_URL] = object["server"].as<String>();
    CONFIG[CITY] = object["city"].as<String>();
    CONFIG[LATITUDE] = object["latitude"].as<String>();
    CONFIG[LONGITUDE] = object["longitude"].as<String>();
    CONFIG[COUNTRY] = object["country"].as<String>();
    CONFIG[LANG] = object["language"].as<String>();
    CONFIG[HEMISPHERE] = object["hemisphere"].as<String>();
    CONFIG[UNITS] = object["units"].as<String>();
    CONFIG[TIMEZONE] = object["timezone"].as<String>();
    CONFIG[NTP_URL] = object["ntpserver"].as<String>();
    Serial.println("DONE");
    store_eeprom();
    file.close();
  }

  void read()
  {
    SPIClass sdSPI(VSPI);
    sdSPI.begin(SDCARD_CLK, SDCARD_MISO, SDCARD_MOSI, SDCARD_SS);
    if (!SD.begin(SDCARD_SS, sdSPI))
    {
      Serial.println("ERROR: SD card mount FAIL");
      read_eeprom();
    }
    else
    {
      Serial.println("INFO: SD card mount PASS");
      File file = SD.open(CONFIG_FILE_NAME, FILE_READ);
      read_sd_card(file);
    }
    dump_confg();
  }

  void dump_confg()
  {
    Serial.print("SSID: ");
    Serial.println(getSsid());
    Serial.print("Password: ");
    Serial.println(getPassword());
    Serial.print("API server: ");
    Serial.println(getServer());
    Serial.print("API Key: ");
    Serial.println(getApiKey());
    Serial.print("City: ");
    Serial.println(getCity());
    Serial.print("Latitude: ");
    Serial.println(getLatitude());
    Serial.print("Longitude: ");
    Serial.println(getLongitude());
    Serial.print("Country: ");
    Serial.println(getCountry());
    Serial.print("Language: ");
    Serial.println(getLanguage());
    Serial.print("Hemisphere: ");
    Serial.println(getHemisphere());
    Serial.print("Units: ");
    Serial.println(getUnits());
    Serial.print("Timezone: ");
    Serial.println(getTimezone());
    Serial.print("NtpServer: ");
    Serial.println(getNtpServer());
  }
}

#endif
