#ifndef COMMON_H_
#define COMMON_H_

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

#include "common.h"

enum RequestType
{
  WEATHER,
  FORECAST
};

String request_type_to_string(const RequestType type)
{
  return type == WEATHER ? "weather" : "forecast";
}

namespace API
{

  void convert_to_imperial()
  {
    WxConditions[0].Pressure = hPa_to_inHg(WxConditions[0].Pressure);
    WxForecast[1].Rainfall = mm_to_inches(WxForecast[1].Rainfall);
    WxForecast[1].Snowfall = mm_to_inches(WxForecast[1].Snowfall);
  }

  // Problems with stucturing JSON decodes, see here: https://arduinojson.org/assistant/
  bool decode_weather(WiFiClient &json, const RequestType type)
  {
    Serial.print(F("\nCreating object...and "));
    // allocate the JsonDocument
    DynamicJsonDocument doc(35 * 1024);
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, json);
    // Test if parsing succeeds.
    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
      return false;
    }
    // convert it to a JsonObject
    JsonObject root = doc.as<JsonObject>();
    Serial.println(" Decoding " + request_type_to_string(type) + " data");
    if (type == WEATHER)
    {
      // All Serial.println statements are for diagnostic purposes and not required, remove if not needed
      WxConditions[0].lon = root["coord"]["lon"].as<float>();
      Serial.println(" Lon: " + String(WxConditions[0].lon));
      WxConditions[0].lat = root["coord"]["lat"].as<float>();
      Serial.println(" Lat: " + String(WxConditions[0].lat));
      WxConditions[0].Main0 = root["weather"][0]["main"].as<char *>();
      Serial.println("Main: " + String(WxConditions[0].Main0));
      WxConditions[0].Forecast0 = root["weather"][0]["description"].as<char *>();
      Serial.println("For0: " + String(WxConditions[0].Forecast0));
      WxConditions[0].Forecast1 = root["weather"][1]["description"].as<char *>();
      Serial.println("For1: " + String(WxConditions[0].Forecast1));
      WxConditions[0].Forecast2 = root["weather"][2]["description"].as<char *>();
      Serial.println("For2: " + String(WxConditions[0].Forecast2));
      WxConditions[0].Icon = root["weather"][0]["icon"].as<char *>();
      Serial.println("Icon: " + String(WxConditions[0].Icon));
      WxConditions[0].Temperature = root["main"]["temp"].as<float>();
      Serial.println("Temp: " + String(WxConditions[0].Temperature));
      WxConditions[0].Pressure = root["main"]["pressure"].as<float>();
      Serial.println("Pres: " + String(WxConditions[0].Pressure));
      WxConditions[0].Humidity = root["main"]["humidity"].as<float>();
      Serial.println("Humi: " + String(WxConditions[0].Humidity));
      WxConditions[0].Low = root["main"]["temp_min"].as<float>();
      Serial.println("TLow: " + String(WxConditions[0].Low));
      WxConditions[0].High = root["main"]["temp_max"].as<float>();
      Serial.println("THig: " + String(WxConditions[0].High));
      WxConditions[0].Windspeed = root["wind"]["speed"].as<float>();
      Serial.println("WSpd: " + String(WxConditions[0].Windspeed));
      WxConditions[0].Winddir = root["wind"]["deg"].as<float>();
      Serial.println("WDir: " + String(WxConditions[0].Winddir));
      WxConditions[0].Cloudcover = root["clouds"]["all"].as<int>();
      Serial.println("CCov: " + String(WxConditions[0].Cloudcover)); // in % of cloud cover
      WxConditions[0].Visibility = root["visibility"].as<int>();
      Serial.println("Visi: " + String(WxConditions[0].Visibility)); // in metres
      WxConditions[0].Rainfall = root["rain"]["1h"].as<float>();
      Serial.println("Rain: " + String(WxConditions[0].Rainfall));
      WxConditions[0].Snowfall = root["snow"]["1h"].as<float>();
      Serial.println("Snow: " + String(WxConditions[0].Snowfall));
      WxConditions[0].Country = root["sys"]["country"].as<char *>();
      Serial.println("Ctry: " + String(WxConditions[0].Country));
      WxConditions[0].Sunrise = root["sys"]["sunrise"].as<int>();
      Serial.println("SRis: " + String(WxConditions[0].Sunrise));
      WxConditions[0].Sunset = root["sys"]["sunset"].as<int>();
      Serial.println("SSet: " + String(WxConditions[0].Sunset));
      WxConditions[0].Timezone = root["timezone"].as<int>();
      Serial.println("TZon: " + String(WxConditions[0].Timezone));
    }
    if (type == FORECAST)
    {
      //Serial.println(json);
      Serial.print(F("\nReceiving Forecast period - ")); //------------------------------------------------
      JsonArray list = root["list"];
      for (byte r = 0; r < MAX_READINGS; r++)
      {
        Serial.println("\nPeriod-" + String(r) + "--------------");
        WxForecast[r].Dt = list[r]["dt"].as<int>();
        Serial.println("DTim: " + String(WxForecast[r].Dt));
        WxForecast[r].Temperature = list[r]["main"]["temp"].as<float>();
        Serial.println("Temp: " + String(WxForecast[r].Temperature));
        WxForecast[r].Low = list[r]["main"]["temp_min"].as<float>();
        Serial.println("TLow: " + String(WxForecast[r].Low));
        WxForecast[r].High = list[r]["main"]["temp_max"].as<float>();
        Serial.println("THig: " + String(WxForecast[r].High));
        WxForecast[r].Pressure = list[r]["main"]["pressure"].as<float>();
        Serial.println("Pres: " + String(WxForecast[r].Pressure));
        WxForecast[r].Humidity = list[r]["main"]["humidity"].as<float>();
        Serial.println("Humi: " + String(WxForecast[r].Humidity));
        WxForecast[r].Forecast0 = list[r]["weather"][0]["main"].as<char *>();
        Serial.println("For0: " + String(WxForecast[r].Forecast0));
        WxForecast[r].Forecast1 = list[r]["weather"][1]["main"].as<char *>();
        Serial.println("For1: " + String(WxForecast[r].Forecast1));
        WxForecast[r].Forecast2 = list[r]["weather"][2]["main"].as<char *>();
        Serial.println("For2: " + String(WxForecast[r].Forecast2));
        WxForecast[r].Icon = list[r]["weather"][0]["icon"].as<char *>();
        Serial.println("Icon: " + String(WxForecast[r].Icon));
        WxForecast[r].Description = list[r]["weather"][0]["description"].as<char *>();
        Serial.println("Desc: " + String(WxForecast[r].Description));
        WxForecast[r].Cloudcover = list[r]["clouds"]["all"].as<int>();
        Serial.println("CCov: " + String(WxForecast[r].Cloudcover)); // in % of cloud cover
        WxForecast[r].Windspeed = list[r]["wind"]["speed"].as<float>();
        Serial.println("WSpd: " + String(WxForecast[r].Windspeed));
        WxForecast[r].Winddir = list[r]["wind"]["deg"].as<float>();
        Serial.println("WDir: " + String(WxForecast[r].Winddir));
        WxForecast[r].Rainfall = list[r]["rain"]["3h"].as<float>();
        Serial.println("Rain: " + String(WxForecast[r].Rainfall));
        WxForecast[r].Snowfall = list[r]["snow"]["3h"].as<float>();
        Serial.println("Snow: " + String(WxForecast[r].Snowfall));
        WxForecast[r].Pop = list[r]["pop"].as<float>();
        Serial.println("Pop:  " + String(WxForecast[r].Pop));
        WxForecast[r].Period = list[r]["dt_txt"].as<char *>();
        Serial.println("Peri: " + String(WxForecast[r].Period));
      }
      //------------------------------------------
      float pressure_trend = WxForecast[0].Pressure - WxForecast[2].Pressure; // Measure pressure slope between ~now and later
      pressure_trend = ((int)(pressure_trend * 10)) / 10.0;                   // Remove any small variations less than 0.1
      WxConditions[0].Trend = "0";
      if (pressure_trend > 0)
        WxConditions[0].Trend = "+";
      if (pressure_trend < 0)
        WxConditions[0].Trend = "-";
      if (pressure_trend == 0)
        WxConditions[0].Trend = "0";

      if (!Config::isMetric())
        convert_to_imperial();
    }
    return true;
  }

  String build_location()
  {
    return Config::getLatitude().isEmpty() || Config::getLatitude().isEmpty()
               ? "?q=" + Config::getCity() + "," + Config::getCountry()
               : "?lat=" + Config::getLatitude() + "&lon=" + Config::getLongitude();
  }

  String build_request_url(const RequestType requestType)
  {
     const String units = (Config::isMetric() ? "metric" : "imperial");
    return "/data/2.5/" + request_type_to_string(requestType) + build_location() + "&APPID=" + Config::getApiKey() + "&mode=json&units=" + units + "&lang=" + Config::getLanguage();
  }

  bool obtain_wx_data(WiFiClient &client, const RequestType requestType)
  {
    client.stop(); // close connection before sending a new request
    HTTPClient http;
    String uri = build_request_url(requestType);
    Serial.print("request URI: ");
    Serial.println(uri);
    if (requestType == FORECAST)
    {
      uri += "&cnt=" + String(MAX_READINGS);
    }
    //http.begin(uri,test_root_ca); //HTTPS example connection
    http.begin(client, Config::getServer(), 80, uri);
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK)
    {
      if (!decode_weather(http.getStream(), requestType))
        return false;
      client.stop();
      http.end();
      return true;
    }
    else
    {
      Serial.printf("connection failed, error: %s", http.errorToString(httpCode).c_str());
      client.stop();
      http.end();
      return false;
    }
    http.end();
    return true;
  }
}
#endif /* ifndef COMMON_H_ */
