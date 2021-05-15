#ifndef DRAW_FUNCTIONS_H_
#define DRAW_FUNCTIONS_H_

#include "common.h"
#include "display.h"

bool LargeIcon = true,
     SmallIcon = false;

#define Large 7 // For best results use odd numbers
#define Small 3 // For best results use odd numbers
typedef struct {
  int x;
  int y;
  int h;
  int w;
} Bounds;

typedef struct {
  int index;
  int x;
  int y;
  int scale = Large;
  int lineSize = 2;
  bool iconSize;
  bool night;
} DrawContext;


void draw_heading_section();
void draw_main_weather_section();
void draw_forecast(int x, int y, int w, int h, int index);
void draw_battery(int x, int y);
void draw_wx_icon(int x, int y, int index, bool IconSize);
void draw_pressure_trend(int x, int y, float pressure, String slope);
void draw_humidity(int x, int y, float hunidity);
void draw_wind(int x, int y, float angle, float windspeed);
void draw_moon(int x, int y, int dd, int mm, int yy, String hemisphere);

String wind_deg_to_direction(float winddirection);

Bounds draw_string(int x, int y, String text, AlignmentType alignment);
void draw_string_max_width(int x, int y, unsigned int text_width, String text, AlignmentType alignment);
Bounds draw_temp(int x, int y, int size, float temp, const GFXfont *font);


void rain(DrawContext ctx);
void haze(DrawContext ctx);
void sunny(DrawContext ctx);
void few_clouds(DrawContext ctx);
void clouds(DrawContext ctx);
void heavy_clouds(DrawContext ctx);
void shower_rain(DrawContext ctx);
void thunder_storm(DrawContext ctx);
void snow(DrawContext ctx);
void no_data(DrawContext ctx);

void addmoon(DrawContext ctx);
void addcloud(DrawContext ctx, bool white);
void addsun(DrawContext ctx);
void addfog(DrawContext ctx);
void addrain(DrawContext ctx);
void addraindrop(DrawContext ctx);
void addtstorm(DrawContext ctx);
void addsnow(DrawContext ctx);

void arrow(int x, int y, int asize, float aangle, int pwidth, int plength);

String convert_unix_time(int unix_time);

void draw_weather()
{
  #ifdef TEST_DATA
  WxForecast[0].Icon = "09d";
  WxForecast[0].Rainfall = 5.0;
  WxForecast[1].Icon = "10d";
  WxForecast[1].Rainfall = 4.0;
  WxForecast[2].Icon = "10n";
  WxForecast[2].Rainfall = 3.0;
  WxForecast[3].Icon = "10n";
  WxForecast[3].Rainfall = 2.0;
  WxForecast[4].Icon = "10d";
  WxForecast[4].Rainfall = 1.0;
  WxForecast[5].Icon = "50d";
  WxForecast[6].Icon = "11d";
  #endif

  Serial.println("Display Weather");
  draw_heading_section();
  draw_main_weather_section();

  int forecastY = 102;
  int forecastW = 42;

  for (int i = 0; i < MAX_READINGS; i++)
  {
    draw_forecast(i * forecastW, forecastY, forecastW, 46, i + 1); // First  3hr forecast box
  }
  draw_battery(70, 12);
  display_update();
}

void draw_forecast(const int x, const int y, const int w, const int h, const int index)
{
  draw_wx_icon(x + 23, y + 1, index, SmallIcon);
  display.setFont(&FORECAST_FONT);
  draw_string(x + w / 2, y - 14, WxForecast[index].Period.substring(11, 16), CENTER);
  Bounds b = draw_temp(x + 4, y + 19, 1, WxForecast[index].High, &FORECAST_FONT);
  b = draw_string(b.x + b.w + 1, y + 19, "/", LEFT);
  draw_temp(b.x + b.w + 1, y + 19, 1, WxForecast[index].Low, &FORECAST_FONT);
  display.drawLine(x + w, y - 24, x + w, y - 24 + h, FG_COLOR);
}

void draw_heading_section()
{
  display.setFont(&DEFALUT_FONT);
  draw_string(0, 8, time_str, LEFT);
  draw_string(SCREEN_WIDTH, 8, date_str, RIGHT);
  display.drawLine(0, 11, SCREEN_WIDTH, 11, FG_COLOR);
}

void draw_main_weather_section()
{
  draw_wx_icon(162, 49, 0, LargeIcon);
  display.setFont(&CITY_FONT);
  draw_string(0, 26, Config::getCity(), LEFT);
  draw_temp(0, 53, 2, WxConditions[0].Temperature, &TEMP_FONT);
  display.setFont(&WEATHER_FONT);
  draw_pressure_trend(0, 64, WxConditions[0].Pressure, WxConditions[0].Trend);
  draw_humidity(60, 72, WxConditions[0].Humidity);
  draw_wind(222, 44, WxConditions[0].Winddir + 180, WxConditions[0].Windspeed);
  display.drawLine(0, 77, SCREEN_WIDTH, 77, FG_COLOR);
}

Bounds draw_temp(int x, int y, int size, float temp, const GFXfont *font)
{
  display.setFont(font);
  Bounds bounds = draw_string(x, y, String(temp, 0), LEFT);
  display.drawCircle(bounds.x + 1.2 * bounds.w, bounds.y + size, size, FG_COLOR);
  bounds.w = 1.2 * bounds.w + size;
  return bounds;
}

void draw_wind(int x, int y, float angle, float windspeed)
{
#define Cradius 15
  float dx = Cradius * cos((angle - 90) * PI / 180) + x; // calculate X position
  float dy = Cradius * sin((angle - 90) * PI / 180) + y; // calculate Y position
  arrow(x, y, Cradius - 3, angle, 10, 12);               // Show wind direction on outer circle
  display.drawCircle(x, y, Cradius + 2, FG_COLOR);
  display.drawCircle(x, y, Cradius + 3, FG_COLOR);
  for (int m = 0; m < 360; m = m + 45)
  {
    dx = Cradius * cos(m * PI / 180); // calculate X position
    dy = Cradius * sin(m * PI / 180); // calculate Y position
    display.drawLine(x + dx, y + dy, x + dx * 0.8, y + dy * 0.8, FG_COLOR);
  }
  display.setFont(&WEATHER_FONT);
  draw_string(x, y + Cradius + 15, wind_deg_to_direction(angle + 180), CENTER);
  draw_string(x, y - Cradius - 7, String(windspeed, 1) + (Config::isMetric() ? " m/s" : " mph"), CENTER);
}
//#########################################################################################
String wind_deg_to_direction(float winddirection)
{
  if (winddirection > 360) {
    winddirection -= 360;
  } else if (winddirection < 0) {
    winddirection += 360;
  }
  if (winddirection >= 348.75 || winddirection < 11.25)
    return TXT_N;
  if (winddirection >= 11.25 && winddirection < 33.75)
    return TXT_NNE;
  if (winddirection >= 33.75 && winddirection < 56.25)
    return TXT_NE;
  if (winddirection >= 56.25 && winddirection < 78.75)
    return TXT_ENE;
  if (winddirection >= 78.75 && winddirection < 101.25)
    return TXT_E;
  if (winddirection >= 101.25 && winddirection < 123.75)
    return TXT_ESE;
  if (winddirection >= 123.75 && winddirection < 146.25)
    return TXT_SE;
  if (winddirection >= 146.25 && winddirection < 168.75)
    return TXT_SSE;
  if (winddirection >= 168.75 && winddirection < 191.25)
    return TXT_S;
  if (winddirection >= 191.25 && winddirection < 213.75)
    return TXT_SSW;
  if (winddirection >= 213.75 && winddirection < 236.25)
    return TXT_SW;
  if (winddirection >= 236.25 && winddirection < 258.75)
    return TXT_WSW;
  if (winddirection >= 258.75 && winddirection < 281.25)
    return TXT_W;
  if (winddirection >= 281.25 && winddirection < 303.75)
    return TXT_WNW;
  if (winddirection >= 303.75 && winddirection < 326.25)
    return TXT_NW;
  if (winddirection >= 326.25 && winddirection < 348.75)
    return TXT_NNW;
  return "?";
}
//#########################################################################################
void arrow(int x, int y, int asize, float aangle, int pwidth, int plength)
{
  // x,y is the centre poistion of the arrow and asize is the radius out from the x,y position
  // aangle is angle to draw the pointer at e.g. at 45В° for NW
  // pwidth is the pointer width in pixels
  // plength is the pointer length in pixels
  float dx = (asize - 10) * cos((aangle - 90) * PI / 180) + x; // calculate X position
  float dy = (asize - 10) * sin((aangle - 90) * PI / 180) + y; // calculate Y position
  float x1 = 0;
  float y1 = plength;
  float x2 = pwidth / 2;
  float y2 = pwidth / 2;
  float x3 = -pwidth / 2;
  float y3 = pwidth / 2;
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
void draw_pressure_trend(int x, int y, float pressure, String slope)
{
  float pressureHgMm = hPa_to_inHg(pressure);
  draw_string(x, y + 8, String(pressureHgMm, 0) + "mm", LEFT);
  x = x + 43;
  y = y + 3;
  if (slope == "+")
  {
    display.drawLine(x, y, x + 4, y - 4, FG_COLOR);
    display.drawLine(x + 4, y - 4, x + 8, y, FG_COLOR);
  }
  else if (slope == "0")
  {
    display.drawLine(x + 3, y - 4, x + 8, y, FG_COLOR);
    display.drawLine(x + 3, y + 4, x + 8, y, FG_COLOR);
  }
  else if (slope == "-")
  {
    display.drawLine(x, y, x + 4, y + 4, FG_COLOR);
    display.drawLine(x + 4, y + 4, x + 8, y, FG_COLOR);
  }
}
//#########################################################################################
void draw_humidity(int x, int y, float hunidity)
{
  draw_string(x, y, String(hunidity, 0) + "%", LEFT);
}
//#########################################################################################
void draw_wx_icon(const int x, const int y, const int index, const bool IconSize)
{
  String IconName = String(WxForecast[index].Icon);
  Serial.println("Icon name: " + IconName);
  
  DrawContext ctx;
  ctx.index = index;
  ctx.x = x;
  ctx.y = y;
  ctx.iconSize = IconSize;
  ctx.night = IconName.endsWith("n");
  ctx.lineSize = IconSize == LargeIcon ? 2 : 1;
  ctx.scale = IconSize == LargeIcon ? Large : Small;
  
  String icon = IconName.substring(0, 2);

  if (icon == "01")
    sunny(ctx);
  else if (icon == "02")
    few_clouds(ctx);
  else if (icon == "03")
    clouds(ctx);
  else if (icon == "04")
    heavy_clouds(ctx);
  else if (icon == "09")
    shower_rain(ctx);
  else if (icon == "10")
    rain(ctx);
  else if (icon == "11")
    thunder_storm(ctx);
  else if (icon == "13")
    snow(ctx);
  else if (icon == "50")
    haze(ctx);
  else
    no_data(ctx);
}

void draw_battery(const int x, int y)
{
  float minLiPoV = 3.4;
  float maxLiPoV = 4.1;
  float percentage = 1.0;
  // analog value = Vbat / 2
  int voltageRaw = 0;
  for (int i = 0; i < 10; i++)
  {
    voltageRaw += analogRead(35);
    delay(10);
  }
  voltageRaw /= 10;
  // voltage = divider * V_ref / Max_Analog_Value
  float voltage = VOLTAGE_DIVIDER_RATIO * voltageRaw / 4096.0;
  if (voltage > 1)
  { // Only display if there is a valid reading
    Serial.println("Voltage Raw = " + String(voltageRaw));
    Serial.println("Voltage = " + String(voltage));
    percentage = (voltage - minLiPoV) / (maxLiPoV - minLiPoV);
    if (voltage >= maxLiPoV)
      percentage = 1;
    if (voltage <= minLiPoV)
      percentage = 0;
    display.drawRect(x, y - 12, 19, 10, FG_COLOR);
    display.fillRect(x + 2, y - 10, 15 * percentage, 6, FG_COLOR);
    display.setFont(&DEFALUT_FONT);
    if (SHOW_PERCENT_VOLTAGE)
    {
      draw_string(x + 21, y - 4, String(percentage * 100, 0) + "%", LEFT);
    } else {
      draw_string(x + 21, y - 4, String(voltage, 1) + "v", LEFT);
    }
    
  }
}

// Symbols are drawn on a relative 10x10grid and 1 scale unit = 1 drawing unit
void addcloud(DrawContext ctx, bool white)
{
  const int x = ctx.x;
  const int y = ctx.y;
  const int scale = ctx.scale;
  const int linesize = ctx.lineSize;
  //Draw cloud outer
  display.fillCircle(x - scale * 3, y, scale, FG_COLOR);                              // Left most circle
  display.fillCircle(x + scale * 3, y, scale, FG_COLOR);                              // Right most circle
  display.fillCircle(x - scale, y - scale, scale * 1.4, FG_COLOR);                    // left middle upper circle
  display.fillCircle(x + scale * 1.5, y - scale * 1.3, scale * 1.75, FG_COLOR);       // Right middle upper circle
  display.fillRect(x - scale * 3 - 1, y - scale, scale * 6, scale * 2 + 1, FG_COLOR); // Upper and lower lines
  if (white)
  {
    //Clear cloud inner
    display.fillCircle(x - scale * 3, y, scale - linesize, BG_COLOR);                                                   // Clear left most circle
    display.fillCircle(x + scale * 3, y, scale - linesize, BG_COLOR);                                                   // Clear right most circle
    display.fillCircle(x - scale, y - scale, scale * 1.4 - linesize, BG_COLOR);                                         // left middle upper circle
    display.fillCircle(x + scale * 1.5, y - scale * 1.3, scale * 1.75 - linesize, BG_COLOR);                            // Right middle upper circle
    display.fillRect(x - scale * 3 + 2, y - scale + linesize - 1, scale * 5.9, scale * 2 - linesize * 2 + 2, BG_COLOR); // Upper and lower lines
  }
}

void addraindrop(const int x, const int y, const int scale)
{
  display.drawLine(x, y, x - scale/2, y + 2 * scale, FG_COLOR);
}

void addrain(DrawContext ctx)
{
  int x = ctx.x;
  int y = ctx.y - ctx.lineSize;
  int scale = ctx.scale;
  int flakes = ceil( WxForecast[ctx.index].Rainfall );
  if (flakes > 5) flakes = 5;

  if (ctx.iconSize == LargeIcon)
    display.setFont(&TEMP_FONT);
  else
    display.setFont(&FORECAST_FONT);

  Bounds b;
  if (flakes > 4)
    b = draw_string(x, y + 4 * scale, "/////", CENTER);
  else if (flakes > 3)
    b = draw_string(x, y + 4 * scale, "////", CENTER);
  else if (flakes > 2)
    b = draw_string(x, y + 4 * scale, "/ / /", CENTER);
  else if (flakes > 1)
    b = draw_string(x, y + 4 * scale, "  / /", CENTER);
  else 
    b = draw_string(x, y + 4 * scale, "   /", CENTER);

  display.fillRect(b.x, y + 4 * scale, b.w, b.h / 3, BG_COLOR);
  display.fillTriangle(b.x, y + 4 * scale, b.x + b.w, y + 4 * scale, b.x, y + 2.7 * scale, BG_COLOR);
}

void addsnow(DrawContext ctx)
{
  int x = ctx.x;
  int y = ctx.y;
  int scale = ctx.scale;
  int flakes = ceil( WxForecast[ctx.index].Snowfall );
  if (flakes > 4) flakes = 4;

  if (ctx.iconSize == LargeIcon)
    display.setFont(&TEMP_FONT);
  else
    display.setFont(&FORECAST_FONT);

  if (flakes > 3)
    draw_string(x, y + 4 * scale, "****", CENTER);
  else if (flakes > 2)
    draw_string(x, y + 4 * scale, "* * *", CENTER);
  else if (flakes > 1)
    draw_string(x, y + 4 * scale, "* *", CENTER);
  else 
    draw_string(x, y + 4 * scale, "*", CENTER);
}

void addtstorm(DrawContext ctx)
{
  int x = ctx.x + ctx.scale;
  int y = ctx.y;
  int scale = ctx.scale * 1.7;
  
  display.drawLine(x, y, x - scale / 2, y + scale, FG_COLOR);
  display.drawLine(x - 1, y, x - scale / 2 - 1, y + scale, FG_COLOR);
  if (ctx.iconSize == LargeIcon) 
  {
    display.drawLine(x + 1, y, x - scale / 2, y + scale, FG_COLOR);
  }
  
  display.drawLine(x - scale / 2, y + scale, x, y + scale, FG_COLOR);
  if (ctx.iconSize == LargeIcon) 
  {
    display.drawLine(x - scale / 2 - 1, y + scale + 1, x - 1, y + scale + 1, FG_COLOR);
  }

  display.drawLine(x, y + scale, x - scale / 2, y + 2 * scale, FG_COLOR);
  display.drawLine(x - 1, y + scale + 1, x - scale / 2, y + 2 * scale, FG_COLOR);
  if (ctx.iconSize == LargeIcon) 
  {
    display.drawLine(x + 1, y + scale, x - scale / 2, y + 2 * scale, FG_COLOR);
  }
  
}

void addsun(DrawContext ctx)
{
  int x = ctx.x;
  int y = ctx.y;
  int scale = ctx.scale;
  int linesize = ctx.lineSize;
  
  display.drawLine(x - scale * 1.8, y, x + scale * 1.8, y, FG_COLOR);
  display.drawLine(x, y + scale * 1.8, x, y - scale * 1.8, FG_COLOR);

  display.drawLine(x - scale * 1.35, y - scale * 1.35, x + scale * 1.35, y + scale * 1.35, FG_COLOR);
  display.drawLine(x - scale * 1.35, y + scale * 1.35, x + scale * 1.35, y - scale * 1.35, FG_COLOR);

  if (ctx.iconSize == LargeIcon)
  {
    display.drawLine(x - scale * 1.8+1, y-1, x + scale * 1.8-1, y-1, FG_COLOR);
    display.drawLine(x - scale * 1.8+1, y+1, x + scale * 1.8-1, y+1, FG_COLOR);

    display.drawLine(x-1, y + scale * 1.8-1, x-1, y - scale * 1.8+1, FG_COLOR);
    display.drawLine(x+1, y + scale * 1.8-1, x+1, y - scale * 1.8+1, FG_COLOR);

    display.drawLine(x - scale * 1.35, y - scale * 1.35 + 1, x + scale * 1.35 - 1, y + scale * 1.35, FG_COLOR);
    display.drawLine(x - scale * 1.35 + 1, y - scale * 1.35, x + scale * 1.35, y + scale * 1.35 - 1, FG_COLOR);

    display.drawLine(x - scale * 1.35 + 1, y + scale * 1.35, x + scale * 1.35, y - scale * 1.35 + 1, FG_COLOR);
    display.drawLine(x - scale * 1.35, y + scale * 1.35 - 1, x + scale * 1.35 - 1, y - scale * 1.35, FG_COLOR);
  }
  if (scale > Large) 
  {
    display.fillCircle(x, y, scale + linesize, BG_COLOR);
  }
  display.fillCircle(x, y, scale, FG_COLOR);
  display.fillCircle(x, y, scale - linesize, BG_COLOR);
}

void addfog(DrawContext ctx)
{
  for (int i = 0; i < 6; i++)
  {
    display.fillRect(ctx.x - ctx.scale    , ctx.y - ctx.scale * 2, ctx.scale * 2, ctx.lineSize, FG_COLOR);
    display.fillRect(ctx.x - ctx.scale * 2, ctx.y - ctx.scale    , ctx.scale * 4, ctx.lineSize, FG_COLOR);
    display.fillRect(ctx.x - ctx.scale * 3, ctx.y                , ctx.scale * 6, ctx.lineSize, FG_COLOR);
    display.fillRect(ctx.x - ctx.scale * 4, ctx.y + ctx.scale    , ctx.scale * 8, ctx.lineSize, FG_COLOR);
  }
}


DrawContext offsetXYS(DrawContext ctx, int x, int y, float scale) {
  DrawContext off = ctx;
  off.x = off.x + x;
  off.y = off.y + y;
  off.scale = off.scale * scale;
  return off;
}

DrawContext offsetXY(DrawContext ctx, int x, int y) {
  DrawContext off = ctx;
  off.x = off.x + x;
  off.y = off.y + y;
  return off;
}

DrawContext offsetX(DrawContext ctx, int x) {
  DrawContext off = ctx;
  off.x = off.x + x;
  return off;
}

DrawContext offsetY(DrawContext ctx, int y) {
  DrawContext off = ctx;
  off.y = off.y + y;
  return off;
}

DrawContext offsetMoon(DrawContext ctx) {
  return offsetXY(ctx, -ctx.scale * 3.6, -ctx.scale * 2.8);
}

void sunny(DrawContext ctx)
{
  if (ctx.night)
    addmoon( offsetXYS(ctx, -0.7 * ctx.scale, -0.7 * ctx.scale, 2.34) );
  else
    addsun( offsetXYS(ctx, -0.7 * ctx.scale, -0.7 * ctx.scale, 2.34) );
}

void few_clouds(DrawContext ctx)
{
  addcloud( offsetXYS(ctx, 0, 0, 0.9), true );
  if (ctx.night)
  {
    addmoon( offsetMoon(ctx) );
  }
  else
  {
    addsun( offsetMoon(ctx) );
  }
}

void clouds(DrawContext ctx)
{
  if (ctx.night)
  {
    addmoon( offsetMoon(ctx) );
  }
  else
  {
    addsun( offsetMoon(ctx) );
  }
  addcloud(ctx, true);
}

void heavy_clouds(DrawContext ctx)
{
  addcloud( offsetXYS(ctx, -4, -5, 0.9), false );
  addcloud( offsetX(ctx, 3), true );
}

void rain(DrawContext ctx)
{
  if (ctx.night)
    addmoon( offsetMoon(ctx) );
  addcloud(ctx, true);
  addrain(ctx);
}

void shower_rain(DrawContext ctx)
{
  heavy_clouds(ctx);
  addrain( offsetX(ctx, 3) );
}

void thunder_storm(DrawContext ctx)
{
  heavy_clouds(ctx);
  addtstorm(ctx);
}

void snow(DrawContext ctx)
{
  if (ctx.night)
    addmoon( offsetMoon(ctx) );
  addcloud(ctx, true);
  addsnow(ctx);
}

void haze(DrawContext ctx)
{
  if (ctx.night)
    addmoon( offsetMoon(ctx) );
  else
    addsun( offsetMoon(ctx) );
  addfog(offsetY(ctx, 3));
}

void Visibility(int x, int y, String Visi)
{
  y = y - 3; //
  float start_angle = 0.52, end_angle = 2.61;
  int r = 10;
  for (float i = start_angle; i < end_angle; i = i + 0.05)
  {
    display.drawPixel(x + r * cos(i), y - r / 2 + r * sin(i), FG_COLOR);
    display.drawPixel(x + r * cos(i), 1 + y - r / 2 + r * sin(i), FG_COLOR);
  }
  start_angle = 3.61;
  end_angle = 5.78;
  for (float i = start_angle; i < end_angle; i = i + 0.05)
  {
    display.drawPixel(x + r * cos(i), y + r / 2 + r * sin(i), FG_COLOR);
    display.drawPixel(x + r * cos(i), 1 + y + r / 2 + r * sin(i), FG_COLOR);
  }
  display.fillCircle(x, y, r / 4, FG_COLOR);
  display.setFont(&DEFALUT_FONT);
  draw_string(x - 10, y + 23, Visi, LEFT);
}

void addmoon(DrawContext ctx)
{
  display.fillCircle(ctx.x, ctx.y, ctx.scale, FG_COLOR);
  display.fillCircle(ctx.x + ctx.scale * 0.9, ctx.y, ctx.scale, BG_COLOR);
}

void no_data(DrawContext ctx)
{
  draw_string(ctx.x - 3, ctx.y, "?", CENTER);
}

Bounds draw_string(int x, int y, String text, AlignmentType alignment)
{
  int16_t x1, y1; //the bounds of x,y and w and h of the variable 'text' in pixels.
  uint16_t w, h;
  display.setTextWrap(false);
  display.getTextBounds(text, x, y, &x1, &y1, &w, &h);
  if (alignment == RIGHT) 
  {
    x = x - w;
    x1 = x1 - w;
  }
  if (alignment == CENTER)
  {
    x = x - w / 2;
    x1 = x1 - w / 2;
  }
  display.setCursor(x, y);
  Bounds b;
  b.x = x1;
  b.y = y1;
  b.w = w;
  b.h = h;
  display.print(text);
  return b;
}

void draw_string_max_width(int x, int y, unsigned int text_width, String text, AlignmentType alignment)
{
  int16_t x1, y1; //the bounds of x,y and w and h of the variable 'text' in pixels.
  uint16_t w, h;
  display.getTextBounds(text, x, y, &x1, &y1, &w, &h);
  if (alignment == RIGHT)
    x = x - w;
  if (alignment == CENTER)
    x = x - w / 2;
  display.setCursor(x, y);
  if (text.length() > text_width * 2)
  {
    display.setFont(&DEFALUT_FONT);
    text_width = 42;
    y = y - 3;
  }
  display.println(text.substring(0, text_width));
  if (text.length() > text_width)
  {
    display.setCursor(x, y + h + 15);
    String secondLine = text.substring(text_width);
    secondLine.trim(); // Remove any leading spaces
    display.println(secondLine);
  }
}

String convert_unix_time(int unix_time)
{
  // Returns either '21:12  ' or ' 09:12pm' depending on Units mode
  time_t tm = unix_time;
  struct tm *now_tm = localtime(&tm);
  char output[40];
  if (Config::isMetric())
  {
    strftime(output, sizeof(output), "%H:%M %d/%m/%y", now_tm);
  }
  else
  {
    strftime(output, sizeof(output), "%I:%M%P %m/%d/%y", now_tm);
  }
  return output;
}

#endif
