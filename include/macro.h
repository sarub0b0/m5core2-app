#pragma once

#include <M5Core2.h>

#ifdef DEBUG
HardwareSerial console(0);  // DEBUG用

#define dprintf(msg...)  \
  do {                   \
    console.printf(msg); \
  } while (0)

#define dprintln(msg) console.println(msg)
#define dprint(msg) console.print(msg)

#else

#define dprintf(msg...)
#define dprintln(msg)
#define dprint(msg)

#endif
