#pragma once

#include <M5Core2.h>
#include <WiFi.h>

#include "macro.h"
#include "time.h"

#define SHORT_DATETIME_FORMAT "%Y %H:%M:%S"
#define LONG_DATETIME_FORMAT "%A, %B %d \n%Y %H:%M:%S"

const char *ntp_server = "ntp.jst.mfeed.ad.jp";

void init_localtime() {
  if (WiFi.status() == WL_CONNECTED) {
    configTime(9 * 3600, 0, ntp_server);
  } else {
    m5.lcd.println("Wi-Fi status is not connected");
    dprintln("Wi-Fi status is not connected");
  }
}

struct tm localtime() {
  struct tm tm;
  if (!getLocalTime(&tm)) {
    dprintln("Failed to obtain time");
  }
  return tm;
}
