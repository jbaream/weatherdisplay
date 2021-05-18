#ifndef SYSTEM_H_
#define SYSTEM_H_

#include <WiFi.h>

#include "config.h"
#include "common.h"
#include "display.h"
#include "lang.h"

uint8_t start_wifi()
{
  Serial.print("\r\nConnecting to: ");
  Serial.print(Config::getSsid());
  Serial.print(":");
  Serial.println(Config::getPassword());
  IPAddress dns(8, 8, 8, 8); // Google DNS
  WiFi.disconnect();
  WiFi.mode(WIFI_STA); // switch off AP
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.begin(Config::getSsid().c_str(), Config::getPassword().c_str());
  unsigned long start = millis();
  uint8_t connectionStatus;
  bool AttemptConnection = true;
  while (AttemptConnection)
  {
    connectionStatus = WiFi.status();
    if (millis() > start + 15000)
    { // Wait 15-secs maximum
      AttemptConnection = false;
    }
    if (connectionStatus == WL_CONNECTED || connectionStatus == WL_CONNECT_FAILED)
    {
      AttemptConnection = false;
    }
    delay(50);
  }
  if (connectionStatus == WL_CONNECTED)
  {
    int wifi_signal = WiFi.RSSI(); // Get Wifi Signal strength now, because the WiFi will be turned off to save power!
    Serial.print("INFO: WiFi strength - ");
    Serial.println(wifi_signal);
    Serial.println("INFO: WiFi connected at: " + WiFi.localIP().toString());
  }
  else
    Serial.println("ERROR: WiFi connection *** FAILED ***");
  return connectionStatus;
}

void stop_wifi()
{
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
}

void begin_sleep()
{
  display_power_off();
  long SleepTimer = (SLEEP_DURATION_MIN * 60 - ((CurrentMin % SLEEP_DURATION_MIN) * 60 + CurrentSec)); //Some ESP32 are too fast to maintain accurate time
  esp_sleep_enable_timer_wakeup((SleepTimer + 20) * 1000000LL);                                        // Added 20-sec extra delay to cater for slow ESP32 RTC timers
#ifdef BUILTIN_LED
  pinMode(BUILTIN_LED, INPUT); // If it's On, turn it off and some boards use GPIO-5 for SPI-SS, which remains low after screen use
  digitalWrite(BUILTIN_LED, LOW);
#endif

  // Disable Flash memory
  digitalWrite(FLASH_CS_PIN, HIGH);
  gpio_deep_sleep_hold_en();

  Serial.println("Entering " + String(SleepTimer) + "-secs of sleep time");
  Serial.println("Awake for : " + String((millis() - StartTime) / 1000.0, 3) + "-secs");
  Serial.println("Starting deep-sleep period...");
  gpio_deep_sleep_hold_en();
  esp_deep_sleep_start(); // Sleep for e.g. 30 minutes
  
}

bool update_local_time()
{
  struct tm timeinfo;
  char time_output[30], day_output[30], update_time[30];
  while (!getLocalTime(&timeinfo, 5000))
  { // Wait for 5-sec for time to synchronise
    Serial.println("ERROR: Failed to obtain time");
    return false;
  }
  CurrentHour = timeinfo.tm_hour;
  CurrentMin = timeinfo.tm_min;
  CurrentSec = timeinfo.tm_sec;
  //See http://www.cplusplus.com/reference/ctime/strftime/
  Serial.println(&timeinfo, "%a %b %d %Y   %H:%M"); // Displays: Saturday, June 24 2017 14:05:49
  if (Config::getUnits() == "M")
  {
    if ((Config::getLanguage() == "CZ") || (Config::getLanguage() == "DE") || (Config::getLanguage() == "NL") || (Config::getLanguage() == "PL") || (Config::getLanguage() == "GR"))
    {
      sprintf(day_output, "%s, %02u. %s %04u", weekday_D[timeinfo.tm_wday], timeinfo.tm_mday, month_M[timeinfo.tm_mon], (timeinfo.tm_year) + 1900); // day_output >> So., 23. Juni 2019 <<
    }
    else
    {
      sprintf(day_output, "%s  %02u-%s-%04u", weekday_D[timeinfo.tm_wday], timeinfo.tm_mday, month_M[timeinfo.tm_mon], (timeinfo.tm_year) + 1900);
    }
    strftime(update_time, sizeof(update_time), "%H:%M", &timeinfo); // Creates: '@ 14:05:49'   and change from 30 to 8 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    sprintf(time_output, "%s", update_time);
  }
  else
  {
    strftime(day_output, sizeof(day_output), "%a %b-%d-%Y", &timeinfo); // Creates  'Sat May-31-2019'
    strftime(update_time, sizeof(update_time), "%r", &timeinfo);        // Creates: '@ 02:05:49pm'
    sprintf(time_output, "%s", update_time);
  }
  date_str = day_output;
  time_str = time_output;
  return true;
}

bool setup_time()
{
  configTime(0, 3600, Config::getNtpServer().c_str(), "time.nist.gov"); //(gmtOffset_sec, daylightOffset_sec, ntpServer)
  setenv("TZ", Config::getTimezone().c_str(), 1);                       //setenv()adds the "TZ" variable to the environment with a value TimeZone, only used if set to 1, 0 means no change
  tzset();                                                              // Set the TZ environment variable
  delay(100);
  return update_local_time();
}

#endif //SYSTEM_H_