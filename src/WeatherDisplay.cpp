#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <SPIFFS.h>
#include <FS.h>
#include <Wire.h>

#define FILESYSTEM SPIFFS

#include "common.h"

#include "time.h"
#include "config.h"
#include "api_functions.h"
#include "draw_functions.h"
#include "system.h"

void begin_sleep();
bool update_local_time();
bool setup_time();

void setup()
{
  StartTime = millis();
  Serial.begin(115200);
  setup_pins();
  
  led_on();
  enable_flash_memory();
  
  Config::read();
  Serial.println("Serial OK, connecting Wi-Fi");
  if (start_wifi() == WL_CONNECTED && setup_time() == true)
  {
    //if ((CurrentHour >= WAKEUP_TIME_HR && CurrentHour <= SLEEP_TIME_HR)) {
    display_init(); // Give screen time to display_init by getting weather data!
    byte Attempts = 1;
    bool RxWeather = false, RxForecast = false;
    WiFiClient client; // wifi client object
    while ((!RxWeather || !RxForecast) && Attempts <= 5)
    { // Try up-to 2 time for Weather and Forecast data
      if (!RxWeather)
        RxWeather = API::obtain_wx_data(client, WEATHER);
      if (!RxForecast)
        RxForecast = API::obtain_wx_data(client, FORECAST);
      Attempts++;
    }
    if (RxWeather && RxForecast)
    {              // Only if received both Weather or Forecast proceed
      stop_wifi(); // Reduces power consumption
      update_local_time();
      draw_weather();
    }
    //}
  }
  led_off();
  disable_flash_memory();

  begin_sleep();
}

void loop()
{ // this will never run!
}