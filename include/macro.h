#pragma once

#include <M5Core2.h>

#ifdef DEBUG

#define dprintf(msg...) \
  do {                  \
    Serial.printf(msg); \
  } while (0)

#define dprintln(msg) Serial.println(msg)
#define dprint(msg) Serial.print(msg)

#else

#define dprintf(msg...)
#define dprintln(msg)
#define dprint(msg)

#endif
