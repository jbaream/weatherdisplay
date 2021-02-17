
#include <ArduinoJson.h>     // https://github.com/bblanchon/ArduinoJson
#include <WiFi.h>            // Built-in
#include "time.h"
#include <SPI.h>
#include <SD.h>
#include <SPIFFS.h>
#include <FS.h>


#define FILESYSTEM SPIFFS

#include "config.h"
#include "forecast_record.h"
#include "lang.h"                   // Localisation (English)


#define SCREEN_WIDTH   250
#define SCREEN_HEIGHT  128

enum alignmentType {LEFT, RIGHT, CENTER};

#define EPD_BUSY   4 // to EPD BUSY
#define EPD_CS     5 // to EPD CS
#define EPD_RST   16 // to EPD RST
#define EPD_DC    17 // to EPD DC
#define EPD_SCK   18 // to EPD CLK
#define EPD_MISO  -1 // MISO is not used, as no data from display
#define EPD_MOSI  23 // to EPD DIN

#define SDCARD_SS 13
#define SDCARD_CLK 14
#define SDCARD_MOSI 15
#define SDCARD_MISO 2

//#include <GxEPD2_BW.h>
//#include <GxEPD2_3C.h>

//GxEPD2_BW<GxEPD2_213_B72, GxEPD2_213_B72::HEIGHT> display(GxEPD2_213_B72(/*CS=D8*/ EPD_CS, /*DC=D3*/ EPD_DC, /*RST=D4*/ EPD_RST, /*BUSY=D2*/ EPD_BUSY)); // GDEH0213B72

#include <GxEPD.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>
#include <GxGDE0213B72B/GxGDE0213B72B.h>

#include "arial5pt7b.h"
#include "arial6pt7b.h"
#include "arial7pt7b.h"
#include "arial8pt7b.h"
#include "arial9pt7b.h"

#define DEFALUT_FONT arial6pt7b
#define TEMP_FONT arial9pt7b
#define WEATHER_FONT arial6pt7b
#define FORECAST_FONT arial5pt7b

#define BIG_FONT arial9pt7b
#define SMALL_FONT arial6pt7b

#define FG_COLOR GxEPD_BLACK
#define BG_COLOR GxEPD_WHITE

GxIO_Class io(SPI, EPD_CS, EPD_DC, EPD_RST);
GxEPD_Class display(io, EPD_RST, EPD_BUSY);

bool    LargeIcon = true, SmallIcon = false;
#define Large 7    // For best results use odd numbers
#define Small 3    // For best results use odd numbers
String  time_str, date_str; // strings to hold time and date
int     wifi_signal, CurrentHour = 0, CurrentMin = 0, CurrentSec = 0;
long    StartTime = 0;

//################ PROGRAM VARIABLES and OBJECTS ##########################################
#define max_readings 7

Forecast_record_type  WxConditions[1];
Forecast_record_type  WxForecast[max_readings];

#include "common.h"

long SleepDuration = 30; // Sleep time in minutes, aligned to the nearest minute boundary, so if 30 will always update at 00 or 30 past the hour
int  WakeupTime    = 7;  // Don't wakeup until after 07:00 to save battery power
int  SleepTime     = 23; // Sleep after (23+1) 00:00 to save battery power

//#########################################################################################
void setup() {
  StartTime = millis();
  Serial.begin(115200);
  readConfig();
  Serial.println("Serial OK, connecting Wi-Fi");
  if (StartWiFi() == WL_CONNECTED && SetupTime() == true) {
    //if ((CurrentHour >= WakeupTime && CurrentHour <= SleepTime)) {
    InitialiseDisplay(); // Give screen time to initialise by getting weather data!
    byte Attempts = 1;
    bool RxWeather = false, RxForecast = false;
    WiFiClient client;   // wifi client object
    while ((RxWeather == false || RxForecast == false) && Attempts <= 5) { // Try up-to 2 time for Weather and Forecast data
      if (RxWeather  == false) RxWeather  = obtain_wx_data(client, "weather");
      if (RxForecast == false) RxForecast = obtain_wx_data(client, "forecast");
      Attempts++;
    }
    if (RxWeather && RxForecast) { // Only if received both Weather or Forecast proceed
      StopWiFi(); // Reduces power consumption
      DisplayWeather();
      //display.display(false); // Full screen update mode
    }
    //}
  }
  BeginSleep();
}
//#########################################################################################
void loop() { // this will never run!
}
//#########################################################################################

void readConfig() {
  SPIClass sdSPI(VSPI);
  sdSPI.begin(SDCARD_CLK, SDCARD_MISO, SDCARD_MOSI, SDCARD_SS);
  if (!SD.begin(SDCARD_SS, sdSPI)) {
    Serial.println("ERROR: SD card mount FAIL");
    readConfigFromEEPROM();
  } else {
    Serial.println("INFO: SD card mount PASS");
    File file = SD.open("/wifi_config.txt", FILE_READ);
    readConfigFromFile(file);
  }
}
//#########################################################################################
void BeginSleep() {
  //  display.powerOff();
  long SleepTimer = (SleepDuration * 60 - ((CurrentMin % SleepDuration) * 60 + CurrentSec)); //Some ESP32 are too fast to maintain accurate time
  esp_sleep_enable_timer_wakeup((SleepTimer + 20) * 1000000LL); // Added 20-sec extra delay to cater for slow ESP32 RTC timers
#ifdef BUILTIN_LED
  pinMode(BUILTIN_LED, INPUT); // If it's On, turn it off and some boards use GPIO-5 for SPI-SS, which remains low after screen use
  digitalWrite(BUILTIN_LED, HIGH);
#endif
  Serial.println("Entering " + String(SleepTimer) + "-secs of sleep time");
  Serial.println("Awake for : " + String((millis() - StartTime) / 1000.0, 3) + "-secs");
  Serial.println("Starting deep-sleep period...");
  esp_deep_sleep_start();  // Sleep for e.g. 30 minutes
}
//#########################################################################################
void DisplayWeather() {
  Serial.println("Display Weather");
  UpdateLocalTime();
  Draw_Heading_Section();
  Draw_Main_Weather_Section();

  int forecastY = 102;
  int forecastW = 42;
  for (int i = 0; i < 6; i++) {
    Draw_3hr_Forecast(i * forecastW, forecastY, forecastW, 46, i + 1);    // First  3hr forecast box
  }
  DrawBattery(70, 12);
  display.update();
}
//#########################################################################################
void Draw_3hr_Forecast(int x, int y, int w, int h, int index) {
  DisplayWXicon(x + 23, y + 1, index, SmallIcon);
  display.setFont(&FORECAST_FONT);
  drawString(x + w / 2, y - 22, WxForecast[index].Period.substring(11, 16), CENTER);
  drawString(x + w / 2, y + 11, String(WxForecast[index].High, 0) + "'/" + String(WxForecast[index].Low, 0) + "'", CENTER);
  display.drawLine(x + w, y - 24, x + w, y - 24 + h , FG_COLOR);
}
//#########################################################################################
void Draw_Heading_Section() {
  display.setFont(&DEFALUT_FONT);
  drawString(0, 0, time_str, LEFT);
  drawString(SCREEN_WIDTH, 0, date_str, RIGHT);
  display.drawLine(0, 11, SCREEN_WIDTH, 11, FG_COLOR);
}
//#########################################################################################
void Draw_Main_Weather_Section() {
  DisplayWXicon(162, 45, 0, LargeIcon);
  display.setFont(&TEMP_FONT);
  drawString(0, 17, getCity(), LEFT);
  drawString(0, 40, String(WxConditions[0].Temperature, 0), LEFT);
  display.setFont(&WEATHER_FONT);
  DrawPressureTrend(0, 64, WxConditions[0].Pressure, WxConditions[0].Trend);
  DrawHunidity(60, 64, WxConditions[0].Humidity);
  DrawWind(222, 44, WxConditions[0].Winddir, WxConditions[0].Windspeed);
  display.drawLine(0, 77, SCREEN_WIDTH, 77, FG_COLOR);
}
//#########################################################################################
void DisplayAstronomySection(int x, int y) {
  //display.drawRect(x, y + 13, 168, 52, BG_COLOR);
  //display.drawLine(x, y + 13, x + 168, y + 13 , BG_COLOR);
  display.drawLine(x + 155, y + 13, x + 155, y + 13 + 50, BG_COLOR);
  display.setFont(&DEFALUT_FONT);
  drawString(x + 5, y + 18, ConvertUnixTime(WxConditions[0].Sunrise).substring(0, (getUnits() == "M" ? 5 : 7)) + " " + TXT_SUNRISE, LEFT);
  drawString(x + 5, y + 34, ConvertUnixTime(WxConditions[0].Sunset).substring(0, (getUnits() == "M" ? 5 : 7)) + " " + TXT_SUNSET, LEFT);
  time_t now = time(NULL);
  struct tm * now_utc = gmtime(&now);
  const int day_utc   = now_utc->tm_mday;
  const int month_utc = now_utc->tm_mon + 1;
  const int year_utc  = now_utc->tm_year + 1900;
  drawString(x + 5, y + 50, MoonPhase(day_utc, month_utc, year_utc, getHemisphere()), LEFT);
  DrawMoon(x + 92, y, day_utc, month_utc, year_utc, getHemisphere());
}
//#########################################################################################
String MoonPhase(int d, int m, int y, String hemisphere) {
  int c, e;
  double jd;
  int b;
  if (m < 3) {
    y--;
    m += 12;
  }
  ++m;
  c   = 365.25 * y;
  e   = 30.6  * m;
  jd  = c + e + d - 694039.09;     /* jd is total days elapsed */
  jd /= 29.53059;                        /* divide by the moon cycle (29.53 days) */
  b   = jd;                              /* int(jd) -> b, take integer part of jd */
  jd -= b;                               /* subtract integer part to leave fractional part of original jd */
  b   = jd * 8 + 0.5;                /* scale fraction from 0-8 and round by adding 0.5 */
  b   = b & 7;                           /* 0 and 8 are the same phase so modulo 8 for 0 */
  if (hemisphere == "south") b = 7 - b;
  if (b == 0) return TXT_MOON_NEW;              // New;              0%  illuminated
  if (b == 1) return TXT_MOON_WAXING_CRESCENT;  // Waxing crescent; 25%  illuminated
  if (b == 2) return TXT_MOON_FIRST_QUARTER;    // First quarter;   50%  illuminated
  if (b == 3) return TXT_MOON_WAXING_GIBBOUS;   // Waxing gibbous;  75%  illuminated
  if (b == 4) return TXT_MOON_FULL;             // Full;            100% illuminated
  if (b == 5) return TXT_MOON_WANING_GIBBOUS;   // Waning gibbous;  75%  illuminated
  if (b == 6) return TXT_MOON_THIRD_QUARTER;    // Third quarter;   50%  illuminated
  if (b == 7) return TXT_MOON_WANING_CRESCENT;  // Waning crescent; 25%  illuminated
  return "";
}
//#########################################################################################
void DrawMoon(int x, int y, int dd, int mm, int yy, String hemisphere) {
  const int diameter = 38;
  double Phase = NormalizedMoonPhase(dd, mm, yy);
  hemisphere.toLowerCase();
  if (hemisphere == "south") Phase = 1 - Phase;
  // Draw dark part of moon
  display.fillCircle(x + diameter - 1, y + diameter, diameter / 2 + 1, BG_COLOR);
  const int number_of_lines = 90;
  for (double Ypos = 0; Ypos <= number_of_lines / 2; Ypos++) {
    double Xpos = sqrt(number_of_lines / 2 * number_of_lines / 2 - Ypos * Ypos);
    // Determine the edges of the lighted part of the moon
    double Rpos = 2 * Xpos;
    double Xpos1, Xpos2;
    if (Phase < 0.5) {
      Xpos1 = -Xpos;
      Xpos2 = Rpos - 2 * Phase * Rpos - Xpos;
    }
    else {
      Xpos1 = Xpos;
      Xpos2 = Xpos - 2 * Phase * Rpos + Rpos;
    }
    // Draw light part of moon
    double pW1x = (Xpos1 + number_of_lines) / number_of_lines * diameter + x;
    double pW1y = (number_of_lines - Ypos)  / number_of_lines * diameter + y;
    double pW2x = (Xpos2 + number_of_lines) / number_of_lines * diameter + x;
    double pW2y = (number_of_lines - Ypos)  / number_of_lines * diameter + y;
    double pW3x = (Xpos1 + number_of_lines) / number_of_lines * diameter + x;
    double pW3y = (Ypos + number_of_lines)  / number_of_lines * diameter + y;
    double pW4x = (Xpos2 + number_of_lines) / number_of_lines * diameter + x;
    double pW4y = (Ypos + number_of_lines)  / number_of_lines * diameter + y;
    display.drawLine(pW1x, pW1y, pW2x, pW2y, FG_COLOR);
    display.drawLine(pW3x, pW3y, pW4x, pW4y, FG_COLOR);
  }
  display.drawCircle(x + diameter - 1, y + diameter, diameter / 2, BG_COLOR);
}
//#########################################################################################
void DrawWind(int x, int y, float angle, float windspeed) {
#define Cradius 15
  float dx = Cradius * cos((angle - 90) * PI / 180) + x; // calculate X position
  float dy = Cradius * sin((angle - 90) * PI / 180) + y; // calculate Y position
  arrow(x, y, Cradius - 3, angle, 10, 12); // Show wind direction on outer circle
  display.drawCircle(x, y, Cradius + 2, FG_COLOR);
  display.drawCircle(x, y, Cradius + 3, FG_COLOR);
  for (int m = 0; m < 360; m = m + 45) {
    dx = Cradius * cos(m * PI / 180); // calculate X position
    dy = Cradius * sin(m * PI / 180); // calculate Y position
    display.drawLine(x + dx, y + dy, x + dx * 0.8, y + dy * 0.8, FG_COLOR);
  }
  display.setFont(&WEATHER_FONT);
  drawString(x, y + Cradius + 7, WindDegToDirection(angle), CENTER);
  drawString(x, y - Cradius - 16, String(windspeed, 1) + (getUnits() == "M" ? " m/s" : " mph"), CENTER);
}
//#########################################################################################
String WindDegToDirection(float winddirection) {
  if (winddirection >= 348.75 || winddirection < 11.25)  return TXT_N;
  if (winddirection >=  11.25 && winddirection < 33.75)  return TXT_NNE;
  if (winddirection >=  33.75 && winddirection < 56.25)  return TXT_NE;
  if (winddirection >=  56.25 && winddirection < 78.75)  return TXT_ENE;
  if (winddirection >=  78.75 && winddirection < 101.25) return TXT_E;
  if (winddirection >= 101.25 && winddirection < 123.75) return TXT_ESE;
  if (winddirection >= 123.75 && winddirection < 146.25) return TXT_SE;
  if (winddirection >= 146.25 && winddirection < 168.75) return TXT_SSE;
  if (winddirection >= 168.75 && winddirection < 191.25) return TXT_S;
  if (winddirection >= 191.25 && winddirection < 213.75) return TXT_SSW;
  if (winddirection >= 213.75 && winddirection < 236.25) return TXT_SW;
  if (winddirection >= 236.25 && winddirection < 258.75) return TXT_WSW;
  if (winddirection >= 258.75 && winddirection < 281.25) return TXT_W;
  if (winddirection >= 281.25 && winddirection < 303.75) return TXT_WNW;
  if (winddirection >= 303.75 && winddirection < 326.25) return TXT_NW;
  if (winddirection >= 326.25 && winddirection < 348.75) return TXT_NNW;
  return "?";
}
//#########################################################################################
void arrow(int x, int y, int asize, float aangle, int pwidth, int plength) {
  // x,y is the centre poistion of the arrow and asize is the radius out from the x,y position
  // aangle is angle to draw the pointer at e.g. at 45В° for NW
  // pwidth is the pointer width in pixels
  // plength is the pointer length in pixels
  float dx = (asize - 10) * cos((aangle - 90) * PI / 180) + x; // calculate X position
  float dy = (asize - 10) * sin((aangle - 90) * PI / 180) + y; // calculate Y position
  float x1 = 0;         float y1 = plength;
  float x2 = pwidth / 2;  float y2 = pwidth / 2;
  float x3 = -pwidth / 2; float y3 = pwidth / 2;
  float angle = aangle * PI / 180 - 135;
  float xx1 = x1 * cos(angle) - y1 * sin(angle) + dx;
  float yy1 = y1 * cos(angle) + x1 * sin(angle) + dy;
  float xx2 = x2 * cos(angle) - y2 * sin(angle) + dx;
  float yy2 = y2 * cos(angle) + x2 * sin(angle) + dy;
  float xx3 = x3 * cos(angle) - y3 * sin(angle) + dx;
  float yy3 = y3 * cos(angle) + x3 * sin(angle) + dy;
  display.fillTriangle(xx1, yy1, xx3, yy3, xx2, yy2, FG_COLOR);
}
//#########################################################################################
void DrawPressureTrend(int x, int y, float pressure, String slope) {
  float pressureHgMm = hPa_to_inHg(pressure);
  drawString(x, y, String(pressureHgMm, 0) + "mm", LEFT);
  x = x + 43;
  y = y + 3;
  if (slope == "+") {
    display.drawLine(x,  y, x + 4, y - 4, FG_COLOR);
    display.drawLine(x + 4, y - 4, x + 8, y, FG_COLOR);
  }
  else if (slope == "0") {
    display.drawLine(x + 3, y - 4, x + 8, y, FG_COLOR);
    display.drawLine(x + 3, y + 4, x + 8, y, FG_COLOR);
  }
  else if (slope == "-") {
    display.drawLine(x,  y, x + 4, y + 4, FG_COLOR);
    display.drawLine(x + 4, y + 4, x + 8, y, FG_COLOR);
  }
}
//#########################################################################################
void DrawHunidity(int x, int y, float hunidity) {
  drawString(x, y, String(hunidity, 0) + "%", LEFT);
}
//#########################################################################################
void DisplayWXicon(int x, int y, int index, bool IconSize) {
  String IconName = String(WxForecast[index].Icon);
  Serial.println("Icon name: " + IconName);
  if      (IconName == "01d" || IconName == "01n")  Sunny(x, y, IconSize, IconName);
  else if (IconName == "02d" || IconName == "02n")  MostlySunny(x, y, IconSize, IconName);
  else if (IconName == "03d" || IconName == "03n")  Cloudy(x, y, IconSize, IconName);
  else if (IconName == "04d" || IconName == "04n")  MostlySunny(x, y, IconSize, IconName);
  else if (IconName == "09d" || IconName == "09n")  ChanceRain(x, y, IconSize, IconName);
  else if (IconName == "10d" || IconName == "10n")  Rain(x, y, IconSize, IconName);
  else if (IconName == "11d" || IconName == "11n")  Tstorms(x, y, IconSize, IconName);
  else if (IconName == "13d" || IconName == "13n")  Snow(x, y, IconSize, index);
  else if (IconName == "50d")                       Haze(x, y, IconSize, IconName);
  else if (IconName == "50n")                       Fog(x, y, IconSize, IconName);
  else                                              Nodata(x, y, IconSize, IconName);
}
//#########################################################################################
uint8_t StartWiFi() {
  Serial.print("\r\nConnecting to: ");
  Serial.print(getSsid());
  Serial.print(":");
  Serial.println(getPassword());
  IPAddress dns(8, 8, 8, 8); // Google DNS
  WiFi.disconnect();
  WiFi.mode(WIFI_STA); // switch off AP
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.begin(getSsid().c_str(), getPassword().c_str());
  unsigned long start = millis();
  uint8_t connectionStatus;
  bool AttemptConnection = true;
  while (AttemptConnection) {
    connectionStatus = WiFi.status();
    if (millis() > start + 15000) { // Wait 15-secs maximum
      AttemptConnection = false;
    }
    if (connectionStatus == WL_CONNECTED || connectionStatus == WL_CONNECT_FAILED) {
      AttemptConnection = false;
    }
    delay(50);
  }
  if (connectionStatus == WL_CONNECTED) {
    wifi_signal = WiFi.RSSI(); // Get Wifi Signal strength now, because the WiFi will be turned off to save power!
    Serial.println("WiFi connected at: " + WiFi.localIP().toString());
  }
  else Serial.println("WiFi connection *** FAILED ***");
  return connectionStatus;
}
//#########################################################################################
void StopWiFi() {
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
}
//#########################################################################################
boolean SetupTime() {
  configTime(0, 3600, getNtpServer().c_str(), "time.nist.gov"); //(gmtOffset_sec, daylightOffset_sec, ntpServer)
  setenv("TZ", getTimezone().c_str(), 1);  //setenv()adds the "TZ" variable to the environment with a value TimeZone, only used if set to 1, 0 means no change
  tzset(); // Set the TZ environment variable
  delay(100);
  bool TimeStatus = UpdateLocalTime();
  return TimeStatus;
}
//#########################################################################################
boolean UpdateLocalTime() {
  struct tm timeinfo;
  char   time_output[30], day_output[30], update_time[30];
  while (!getLocalTime(&timeinfo, 5000)) { // Wait for 5-sec for time to synchronise
    Serial.println("Failed to obtain time");
    return false;
  }
  CurrentHour = timeinfo.tm_hour;
  CurrentMin  = timeinfo.tm_min;
  CurrentSec  = timeinfo.tm_sec;
  //See http://www.cplusplus.com/reference/ctime/strftime/
  Serial.println(&timeinfo, "%a %b %d %Y   %H:%M:%S");      // Displays: Saturday, June 24 2017 14:05:49
  if (getUnits() == "M") {
    if ((getLanguage() == "CZ") || (getLanguage() == "DE") || (getLanguage() == "NL") || (getLanguage() == "PL") || (getLanguage() == "GR"))  {
      sprintf(day_output, "%s, %02u. %s %04u", weekday_D[timeinfo.tm_wday], timeinfo.tm_mday, month_M[timeinfo.tm_mon], (timeinfo.tm_year) + 1900); // day_output >> So., 23. Juni 2019 <<
    }
    else
    {
      sprintf(day_output, "%s  %02u-%s-%04u", weekday_D[timeinfo.tm_wday], timeinfo.tm_mday, month_M[timeinfo.tm_mon], (timeinfo.tm_year) + 1900);
    }
    strftime(update_time, sizeof(update_time), "%H:%M:%S", &timeinfo);  // Creates: '@ 14:05:49'   and change from 30 to 8 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
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
//#########################################################################################
void DrawBattery(int x, int y) {
  float minLiPoV = 3.4;
  float maxLiPoV = 4.2;
  float percentage = 1.0;
  // analog value = Vbat / 2
  int voltageRaw = 0;
  for (int i = 0; i < 10; i++) {
    voltageRaw += analogRead(35);
  }
  voltageRaw /= 10;
  // voltage = divider * V_ref / Max_Analog_Value
  float voltage = 2.205 * 3.27 * voltageRaw / 4096.0;
  if (voltage > 1 ) { // Only display if there is a valid reading
    Serial.println("Voltage Raw = " + String(voltageRaw));
    Serial.println("Voltage = " + String(voltage));
    percentage = (voltage - minLiPoV) / (maxLiPoV - minLiPoV);
    if (voltage >= maxLiPoV)
      percentage = 1;
    if (voltage <= minLiPoV)
      percentage = 0;
    display.drawRect(x, y - 12, 19, 10, FG_COLOR);
    //    display.fillRect(x + 19, y - 9, 2, 5, FG_COLOR);
    display.fillRect(x + 2, y - 10, 15 * percentage, 6, FG_COLOR);
    display.setFont(&DEFALUT_FONT);
    //    drawString(x + 60, y - 12, String(percentage * 100) + "%", RIGHT);
    drawString(x + 24, y - 12,  String(voltage, 1) + "v", LEFT);
  }
}
//#########################################################################################
// Symbols are drawn on a relative 10x10grid and 1 scale unit = 1 drawing unit
void addcloud(int x, int y, int scale, int linesize) {
  //Draw cloud outer
  display.fillCircle(x - scale * 3, y, scale, FG_COLOR);                // Left most circle
  display.fillCircle(x + scale * 3, y, scale, FG_COLOR);                // Right most circle
  display.fillCircle(x - scale, y - scale, scale * 1.4, FG_COLOR);    // left middle upper circle
  display.fillCircle(x + scale * 1.5, y - scale * 1.3, scale * 1.75, FG_COLOR); // Right middle upper circle
  display.fillRect(x - scale * 3 - 1, y - scale, scale * 6, scale * 2 + 1, FG_COLOR); // Upper and lower lines
  //Clear cloud inner
  display.fillCircle(x - scale * 3, y, scale - linesize, BG_COLOR);            // Clear left most circle
  display.fillCircle(x + scale * 3, y, scale - linesize, BG_COLOR);            // Clear right most circle
  display.fillCircle(x - scale, y - scale, scale * 1.4 - linesize, BG_COLOR);  // left middle upper circle
  display.fillCircle(x + scale * 1.5, y - scale * 1.3, scale * 1.75 - linesize, BG_COLOR); // Right middle upper circle
  display.fillRect(x - scale * 3 + 2, y - scale + linesize - 1, scale * 5.9, scale * 2 - linesize * 2 + 2, BG_COLOR); // Upper and lower lines
}
//#########################################################################################
void addraindrop(int x, int y, int scale) {
  display.fillCircle(x, y, scale / 2, FG_COLOR);
  display.fillTriangle(x - scale / 2, y, x, y - scale * 1.2, x + scale / 2, y , FG_COLOR);
  x = x + scale * 1.6; y = y + scale / 3;
  display.fillCircle(x, y, scale / 2, FG_COLOR);
  display.fillTriangle(x - scale / 2, y, x, y - scale * 1.2, x + scale / 2, y , FG_COLOR);
}
//#########################################################################################
void addrain(int x, int y, int scale, bool IconSize) {
  if (IconSize == SmallIcon) scale *= 1.34;
  for (int d = 0; d < 4; d++) {
    addraindrop(x + scale * (7.8 - d * 1.95) - scale * 5.2, y + scale * 2.1 - scale / 6, scale / 1.6);
  }
}
//#########################################################################################
void addsnow(int x, int y, int scale, bool IconSize, int index) {
  float volume = WxForecast[index].Snowfall;
  int flakes = ceil(volume);
  if (flakes > 4) flakes = 5;
  int cx, cy;
  float fw = scale * 2;
  cy = y + 2.5 * scale;
  float r = scale * 0.7;
  for (int i = 0; i < flakes; i++) {
    cx = x - fw * flakes / 2  + (i + 0.5) * fw;
    for (int a = 30; a < 360; a += 60) {
      display.drawLine(cx, cy, cx + r * cos(PI * a / 180), cy + r * sin(PI * a / 180), FG_COLOR);
    }
  }
}
//#########################################################################################
void addtstorm(int x, int y, int scale) {
  y = y + scale / 2;
  for (int i = 0; i < 5; i++) {
    display.drawLine(x - scale * 4 + scale * i * 1.5 + 0, y + scale * 1.5, x - scale * 3.5 + scale * i * 1.5 + 0, y + scale, FG_COLOR);
    if (scale != Small) {
      display.drawLine(x - scale * 4 + scale * i * 1.5 + 1, y + scale * 1.5, x - scale * 3.5 + scale * i * 1.5 + 1, y + scale, FG_COLOR);
      display.drawLine(x - scale * 4 + scale * i * 1.5 + 2, y + scale * 1.5, x - scale * 3.5 + scale * i * 1.5 + 2, y + scale, FG_COLOR);
    }
    display.drawLine(x - scale * 4 + scale * i * 1.5, y + scale * 1.5 + 0, x - scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5 + 0, FG_COLOR);
    if (scale != Small) {
      display.drawLine(x - scale * 4 + scale * i * 1.5, y + scale * 1.5 + 1, x - scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5 + 1, FG_COLOR);
      display.drawLine(x - scale * 4 + scale * i * 1.5, y + scale * 1.5 + 2, x - scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5 + 2, FG_COLOR);
    }
    display.drawLine(x - scale * 3.5 + scale * i * 1.4 + 0, y + scale * 2.5, x - scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5, FG_COLOR);
    if (scale != Small) {
      display.drawLine(x - scale * 3.5 + scale * i * 1.4 + 1, y + scale * 2.5, x - scale * 3 + scale * i * 1.5 + 1, y + scale * 1.5, FG_COLOR);
      display.drawLine(x - scale * 3.5 + scale * i * 1.4 + 2, y + scale * 2.5, x - scale * 3 + scale * i * 1.5 + 2, y + scale * 1.5, FG_COLOR);
    }
  }
}
//#########################################################################################
void addsun(int x, int y, int scale, bool IconSize) {
  int linesize = 3;
  if (IconSize == SmallIcon) linesize = 1;
  display.fillRect(x - scale * 2, y, scale * 4, linesize, FG_COLOR);
  display.fillRect(x, y - scale * 2, linesize, scale * 4, FG_COLOR);
  display.drawLine(x - scale * 1.3, y - scale * 1.3, x + scale * 1.3, y + scale * 1.3, FG_COLOR);
  display.drawLine(x - scale * 1.3, y + scale * 1.3, x + scale * 1.3, y - scale * 1.3, FG_COLOR);
  if (IconSize == LargeIcon) {
    display.drawLine(1 + x - scale * 1.3, y - scale * 1.3, 1 + x + scale * 1.3, y + scale * 1.3, FG_COLOR);
    display.drawLine(2 + x - scale * 1.3, y - scale * 1.3, 2 + x + scale * 1.3, y + scale * 1.3, FG_COLOR);
    display.drawLine(3 + x - scale * 1.3, y - scale * 1.3, 3 + x + scale * 1.3, y + scale * 1.3, FG_COLOR);
    display.drawLine(1 + x - scale * 1.3, y + scale * 1.3, 1 + x + scale * 1.3, y - scale * 1.3, FG_COLOR);
    display.drawLine(2 + x - scale * 1.3, y + scale * 1.3, 2 + x + scale * 1.3, y - scale * 1.3, FG_COLOR);
    display.drawLine(3 + x - scale * 1.3, y + scale * 1.3, 3 + x + scale * 1.3, y - scale * 1.3, FG_COLOR);
  }
  display.fillCircle(x, y, scale * 1.3, BG_COLOR);
  display.fillCircle(x, y, scale, FG_COLOR);
  display.fillCircle(x, y, scale - linesize, BG_COLOR);
}
//#########################################################################################
void addfog(int x, int y, int scale, int linesize, bool IconSize) {
  if (IconSize == SmallIcon) {
    linesize = 1;
    y = y - 1;
  } else y = y - 3;
  for (int i = 0; i < 6; i++) {
    display.fillRect(x - scale * 3, y + scale * 1.5, scale * 6, linesize, FG_COLOR);
    display.fillRect(x - scale * 3, y + scale * 2.0, scale * 6, linesize, FG_COLOR);
    display.fillRect(x - scale * 3, y + scale * 2.6, scale * 6, linesize, FG_COLOR);
  }
}
//#########################################################################################
void Sunny(int x, int y, bool IconSize, String IconName) {
  int scale = Small;
  if (IconSize == LargeIcon) {
    scale = Large;
    y = y - 4; // Shift up large sun
  }
  else y = y + 2; // Shift down small sun icon
  if (IconName.endsWith("n"))
    addmoon(x, y + 3, scale, IconSize);
  else
    addsun(x, y, scale, IconSize);
  scale = scale * 1.6;
}
//#########################################################################################
void MostlySunny(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 3, offset = 5;
  if (IconSize == LargeIcon) {
    scale = Large;
    offset = 10;
  }
  if (scale == Small) linesize = 1;
  if (IconName.endsWith("n")) {
    addmoon(x, y + offset + (IconSize ? -8 : 0), scale, IconSize);
    addcloud(x, y + offset, scale, linesize);
  } else {
    addcloud(x, y + offset, scale, linesize);
    addsun(x - scale * 1.8, y - scale * 1.8 + offset, scale, IconSize);
  }
}
//#########################################################################################
void MostlyCloudy(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 3;
  if (IconSize == LargeIcon) {
    scale = Large;
    linesize = 1;
  }
  if (IconName.endsWith("n"))
    addmoon(x, y, scale, IconSize);
  else
    addsun(x - scale * 1.8, y - scale * 1.8, scale, IconSize);
  addcloud(x, y, scale, linesize);
}
//#########################################################################################
void Cloudy(int x, int y, bool IconSize, String IconName) {
  int scale = Large, linesize = 3;
  if (IconSize == SmallIcon) {
    scale = Small;
    if (IconName.endsWith("n"))
      addmoon(x, y, scale, IconSize);
    linesize = 1;
    addcloud(x, y, scale, linesize);
  }
  else {
    y += 12;
    if (IconName.endsWith("n"))
      addmoon(x - 5, y - 15, scale, IconSize);
    addcloud(x + 15, y - 25, 5, linesize); // Cloud top right
    addcloud(x - 15, y - 10, 7, linesize); // Cloud top left
    addcloud(x, y, scale, linesize);       // Main cloud
  }
}
//#########################################################################################
void Rain(int x, int y, bool IconSize, String IconName) {
  int scale = Large, linesize = 3;
  if (IconSize == SmallIcon) {
    scale = Small;
    linesize = 1;
  }
  if (IconName.endsWith("n"))
    addmoon(x, y, scale, IconSize);
  addcloud(x, y, scale, linesize);
  addrain(x, y, scale, IconSize);
}
//#########################################################################################
void ExpectRain(int x, int y, bool IconSize, String IconName) {
  int scale = Large, linesize = 3;
  if (IconSize == SmallIcon) {
    scale = Small;
    linesize = 1;
  }
  if (IconName.endsWith("n"))
    addmoon(x, y, scale, IconSize);
  else
    addsun(x - scale * 1.8, y - scale * 1.8, scale, IconSize);
  addcloud(x, y, scale, linesize);
  addrain(x, y, scale, IconSize);
}
//#########################################################################################
void ChanceRain(int x, int y, bool IconSize, String IconName) {
  int scale = Large, linesize = 3;
  if (IconSize == SmallIcon) {
    scale = Small;
    linesize = 1;
  }
  if (IconName.endsWith("n"))
    addmoon(x - (IconSize ? 8 : 0), y, scale, IconSize);
  else
    addsun(x - scale * 1.8, y - scale * 1.8, scale, IconSize);
  addcloud(x, y, scale, linesize);
  addrain(x, y, scale, IconSize);
}
//#########################################################################################
void Tstorms(int x, int y, bool IconSize, String IconName) {
  int scale = Large, linesize = 3;
  if (IconSize == SmallIcon) {
    scale = Small;
    linesize = 1;
  }
  if (IconName.endsWith("n"))
    addmoon(x, y, scale, IconSize);
  addcloud(x, y, scale, linesize);
  addtstorm(x, y, scale);
}
//#########################################################################################
void Snow(int x, int y, bool IconSize, int index) {
  int scale = Large, linesize = 3;
  if (IconSize == SmallIcon) {
    scale = Small;
    linesize = 1;
  }
  String IconName = String(WxForecast[index].Icon);
  if (IconName.endsWith("n"))
    addmoon(x, y, scale, IconSize);
  addcloud(x, y, scale, linesize);
  addsnow(x, y, scale, IconSize, index);
}
//#########################################################################################
void Fog(int x, int y, bool IconSize, String IconName) {
  int linesize = 3, scale = Large;
  if (IconSize == SmallIcon) {
    scale = Small;
    linesize = 1;
    y = y + 5;
  }
  if (IconName.endsWith("n"))
    addmoon(x, y, scale, IconSize);
  addcloud(x, y - 5, scale, linesize);
  addfog(x, y - 2, scale, linesize, IconSize);
}
//#########################################################################################
void Haze(int x, int y, bool IconSize, String IconName) {
  int linesize = 3, scale = Large;
  if (IconSize == SmallIcon) {
    scale = Small;
    linesize = 1;
  }
  if (IconName.endsWith("n"))
    addmoon(x, y, scale, IconSize);
  else
    addsun(x, y - 2, scale * 1.4, IconSize);
  addfog(x, y + 3 - (IconSize ? 12 : 0), scale * 1.4, linesize, IconSize);
}
//#########################################################################################
void CloudCover(int x, int y, int CCover) {
  addcloud(x - 9, y, Small * 0.8, 2); // Cloud top left
  addcloud(x + 3, y, Small * 0.8, 2); // Cloud top right
  addcloud(x, y + 3, Small * 0.8, 2); // Main cloud
  display.setFont(&DEFALUT_FONT);
  drawString(x + 15, y, String(CCover) + "%", LEFT);
}
//#########################################################################################
void Visibility(int x, int y, String Visi) {
  y = y - 3; //
  float start_angle = 0.52, end_angle = 2.61;
  int r = 10;
  for (float i = start_angle; i < end_angle; i = i + 0.05) {
    display.drawPixel(x + r * cos(i), y - r / 2 + r * sin(i), FG_COLOR);
    display.drawPixel(x + r * cos(i), 1 + y - r / 2 + r * sin(i), FG_COLOR);
  }
  start_angle = 3.61; end_angle = 5.78;
  for (float i = start_angle; i < end_angle; i = i + 0.05) {
    display.drawPixel(x + r * cos(i), y + r / 2 + r * sin(i), FG_COLOR);
    display.drawPixel(x + r * cos(i), 1 + y + r / 2 + r * sin(i), FG_COLOR);
  }
  display.fillCircle(x, y, r / 4, FG_COLOR);
  display.setFont(&DEFALUT_FONT);
  drawString(x - 10, y + 15, Visi, LEFT);
}
//#########################################################################################
void addmoon(int x, int y, int scale, bool IconSize) {
  if (IconSize == LargeIcon) {
    x = x - 5; y = y + 5;
    display.fillCircle(x - 16, y - 23, scale, FG_COLOR);
    display.fillCircle(x - 10, y - 23, scale, BG_COLOR);
  }
  else
  {
    display.fillCircle(x - 10, y - 11, scale, FG_COLOR);
    display.fillCircle(x - 7, y - 11, scale, BG_COLOR);
  }
}
//#########################################################################################
void Nodata(int x, int y, bool IconSize, String IconName) {
  //  if (IconSize == LargeIcon) display.setFont(&DEFALUT_FONT); else display.setFont(&DEFALUT_FONT);
  drawString(x - 3, y - 8, "?", CENTER);
  //  display.setFont(&DEFALUT_FONT);
}
//#########################################################################################
void drawString(int x, int y, String text, alignmentType alignment) {
  int16_t  x1, y1; //the bounds of x,y and w and h of the variable 'text' in pixels.
  uint16_t w, h;
  display.setTextWrap(false);
  display.getTextBounds(text, x, y, &x1, &y1, &w, &h);
  if (alignment == RIGHT)  x = x - w;
  if (alignment == CENTER) x = x - w / 2;
  display.setCursor(x, y + h);
  display.print(text);
}

//#########################################################################################
void drawStringMaxWidth(int x, int y, unsigned int text_width, String text, alignmentType alignment) {
  int16_t  x1, y1; //the bounds of x,y and w and h of the variable 'text' in pixels.
  uint16_t w, h;
  display.getTextBounds(text, x, y, &x1, &y1, &w, &h);
  if (alignment == RIGHT)  x = x - w;
  if (alignment == CENTER) x = x - w / 2;
  display.setCursor(x, y);
  if (text.length() > text_width * 2) {
    display.setFont(&DEFALUT_FONT);
    text_width = 42;
    y = y - 3;
  }
  display.println(text.substring(0, text_width));
  if (text.length() > text_width) {
    display.setCursor(x, y + h + 15);
    String secondLine = text.substring(text_width);
    secondLine.trim(); // Remove any leading spaces
    display.println(secondLine);
  }
}


//#########################################################################################
void InitialiseDisplay() {
  Serial.println("Begin InitialiseDisplay...");
  SPI.begin(EPD_SCK, EPD_MISO, EPD_MOSI, EPD_CS);
  display.init();
  display.setRotation(1);                    // Use 1 or 3 for landscape modes
  display.setTextColor(FG_COLOR);
  display.setFont(&DEFALUT_FONT);
  Serial.println("... End InitialiseDisplay");
}

/*
  Version 6.0 reformatted to use u8g2 fonts
  1.  Screen layout revised
  2.  Made consitent with other versions specifically 7x5 variant
  3.  Introduced Visibility in Metres, Cloud cover in % and RH in %
  4.  Correct sunrise/sunset time when in imperial mode.

  Version 6.1 Provided connection support for Waveshare ESP32 driver board

  Version 6.2 Changed GxEPD2 initialisation from 115200 to 0
  1.  display.init(115200); becomes display.init(0); to stop blank screen following update to GxEPD2

  Version 6.3 changed u8g2 fonts selection
   1.  Omitted 'FONT(' and added _tf to font names either Regular (R) or Bold (B)

  Version 6.4
   1. Added an extra 20-secs sleep delay to allow for slow ESP32 RTC timers

*/
