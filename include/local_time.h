#pragma once

#include <M5Core2.h>
#include <WiFi.h>

#include "macro.h"
#include "time.h"

#define SHORT_DATETIME_FORMAT "%Y %H:%M:%S"
#define LONG_DATETIME_FORMAT "%A, %B %d \n%Y %H:%M:%S"

struct tm timeinfo;

const char *ntp_server = "ntp.jst.mfeed.ad.jp";

const long gmt_offset_sec = 9 * 3600;

const int daylight_offset_sec = 0;

void init_time() {
  if (WiFi.status() != WL_CONNECTED) {
    m5.lcd.println("Wi-Fi status is not connected");
    return;
  }

  configTime(gmt_offset_sec, daylight_offset_sec, ntp_server);
}

int update_local_time(struct tm *tm) {
  if (!getLocalTime(tm)) {
    m5.lcd.println("Failed to obtain time");
    return -1;
  }

  return 0;
}

class LocalTime {
 public:
 private:
};