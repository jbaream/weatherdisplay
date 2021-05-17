
#include <WiFi.h>

#include "config.h"
#include "common.h"

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