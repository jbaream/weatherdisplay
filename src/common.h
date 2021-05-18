#ifndef VARIABLES_H_
#define VARIABLES_H_

#include "forecast_record.h"

#define MAX_READINGS        7   // Max hourly forecast readings
#define SLEEP_DURATION_MIN  30  // Sleep time in minutes, aligned to the nearest minute boundary, so if 30 will always update at 00 or 30 past the hour
#define WAKEUP_TIME_HR      7   // Don't wakeup until after this hour to save battery power
#define SLEEP_TIME_HR       23  // Sleep after this hour to save battery power

#define SHOW_PERCENT_VOLTAGE true

#define VOLTAGE_DIVIDER_RATIO 6.907 //7.0 Varies from board to board

// #define TEST_DATA

ForecastRecord  WxConditions[1];
ForecastRecord  WxForecast[MAX_READINGS];

String  time_str; 
String  date_str; // strings to hold time and date

int CurrentHour = 0;
int CurrentMin = 0;
int CurrentSec = 0;

long    StartTime = 0;

float mm_to_inches(float value_mm)
{
  return 0.0393701 * value_mm;
}

float hPa_to_inHg(float value_hPa)
{
  return 0.750063755419211 * value_hPa;
}

#endif