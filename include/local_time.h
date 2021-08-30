#pragma once

#include <M5Core2.h>
#include <WiFi.h>

#include "macro.h"
#include "time.h"

#define SHORT_DATETIME_FORMAT "%Y %H:%M:%S"
#define LONG_DATETIME_FORMAT "%A, %B %d \n%Y %H:%M:%S"

const char *ntp_server = "ntp.jst.mfeed.ad.jp";

const long gmt_offset_sec = 9 * 3600;

const int daylight_offset_sec = 0;

class LocalTime {
 public:
  LocalTime() {
  }
  ~LocalTime() {
  }

  void begin() {
    if (WiFi.status() == WL_CONNECTED) {
      configTime(gmt_offset_sec, daylight_offset_sec, ntp_server);
    } else {
      m5.lcd.println("Wi-Fi status is not connected");
    }

    return;
  }

  struct tm timeinfo() {
    if (!getLocalTime(&tm_)) {
      m5.lcd.println("Failed to obtain time");
    }
    return tm_;
  }

 private:
  struct tm tm_;
};

LocalTime local_time;
