#include <NTPClient.h>
#include <Rtc_Pcf8563.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <ctime>

#include "constants.h"

// void init()
// DateTime getDateTime()
// void loop()

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
Rtc_Pcf8563 rtc;

// Create a class 'RTCManager' to manage the RTC module including setup, sync
// with NTP on setup and once daily, and provide public functions for getting
// the current time.
class RTCManager {
  public:
  void init() {
    Serial.println("NTP Client init");
    timeClient.begin();
    while (!timeClient.update()) {
      timeClient.forceUpdate();
    }
    Serial.println("RTC Client init");
    rtc.initClock();
    updateRTCwithNTP();
  }
  int getYear() { return rtc.getYear(); }
  int getMonth() { return rtc.getMonth(); }
  int getDay() { return rtc.getDay(); }
  int getHour() { return rtc.getHour(); }
  int getMinute() { return rtc.getMinute(); }
  int getSecond() { return rtc.getSecond(); }
  DateTime getDateTime() {
    DateTime dateTime;
    dateTime.year = rtc.getYear();
    dateTime.month = rtc.getMonth();
    dateTime.dayOfMonth = rtc.getDay();
    dateTime.dayOfWeek = rtc.getWeekday();
    dateTime.hour = rtc.getHour();
    dateTime.minute = rtc.getMinute();
    dateTime.second = rtc.getSecond();
    return dateTime;
  }
  void loop() {
    int today = rtc.getDay();

    // update RTC once a day
    if (lastUpdateDate != today) {
      Serial.println("Updating RTC");
      updateRTCwithNTP();
      lastUpdateDate = today;
    } else {
      Serial.println("RTC up to date");
    }
  }

  private:
  int lastUpdateDate = 0;
  int epochTime;
  DateTime dateTime;
  void updateRTCwithNTP() {
    // Get NTP time
    dateTime = convertEpochToDateTime();
    Serial.println("Updating dateTime:");
    Serial.print("year: ");
    Serial.println(dateTime.year);
    Serial.print("month: ");
    Serial.println(dateTime.month);
    Serial.print("dayOfMonth: ");
    Serial.println(dateTime.dayOfMonth);
    Serial.print("dayOfWeek: ");
    Serial.println(dateTime.dayOfWeek);
    Serial.print("hour: ");
    Serial.println(dateTime.hour);
    Serial.print("minute: ");
    Serial.println(dateTime.minute);
    Serial.print("second: ");
    Serial.println(dateTime.second);

    Serial.println("------------------");

    // Update offset for DaylightSavingsTime as necessary

    if (isDST(dateTime.year, dateTime.month, dateTime.dayOfMonth,
              dateTime.hour)) {
      timeClient.setTimeOffset(-25200);
    } else {
      timeClient.setTimeOffset(-28800);
    }

    rtc.setDate(dateTime.dayOfMonth, dateTime.dayOfWeek, dateTime.month, 0,
                dateTime.year - 2000);
    rtc.setTime(dateTime.hour, dateTime.minute, dateTime.second);
  }
  int dayOfWeek(int year, int month, int day) {
    int y = year;
    byte m = month;
    byte d = day;

    if (m < 3) {
      y--;
      m += 12;
    }

    return (y + y / 4 - y / 100 + y / 400 + (13 * m + 8) / 5 + d) % 7;
  }

  bool isDST(int year, int month, int day, int hour) {
    if (month < 3 || month > 11)
      return false;  // not DST in Jan, Feb, Dec
    if (month > 3 && month < 11)
      return true;  // DST in Apr, May, Jun, Jul, Aug, Sep, Oct
    int previousSunday = day - (dayOfWeek(year, month, day) - 1);
    // in March, we are DST if our previous Sunday was on or after the 8th
    if (month == 3)
      return previousSunday >= 8;
    // in November we must be before the first Sunday to be DST
    // that means the previous Sunday must be before the 1st
    return previousSunday <= 0;
  }
  DateTime convertEpochToDateTime() {
    time_t epochSeconds = timeClient.getEpochTime();
    struct tm* timeInfo = gmtime(&epochSeconds);

    DateTime dateTime;
    dateTime.year = timeInfo->tm_year + 1900;  // Years since 1900
    dateTime.month = timeInfo->tm_mon + 1;     // Months are 0-based
    dateTime.dayOfMonth = timeInfo->tm_mday;   // Day of the month
    dateTime.dayOfWeek = timeInfo->tm_wday;    // Day of the week (0 = Sunday)
    dateTime.hour = timeInfo->tm_hour;         // Hour (0-23)
    dateTime.minute = timeInfo->tm_min;        // Minutes (0-59)
    dateTime.second = timeInfo->tm_sec;        // Seconds (0-59)

    return dateTime;
  }
};