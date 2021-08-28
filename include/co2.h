#pragma once

#include <M5Core2.h>

#include "macro.h"

#define MHZ19_REQUEST_LENGTH 9
#define RETRY_COUNT 5

const uint8_t auto_calibration_off_command[MHZ19_REQUEST_LENGTH] = {
    0xff, 0x01, 0x79, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80};

const uint8_t read_co2_command[MHZ19_REQUEST_LENGTH] = {
    0xff, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};

class MHZ19 {
public:
  MHZ19(int8_t rx_pin, int8_t tx_pin, int uart_nr) : serial_(uart_nr) {
    rx_pin_ = rx_pin;
    tx_pin_ = tx_pin;
  };
  ~MHZ19(){};

  void begin() {
    serial_.begin(9600, SERIAL_8N1, rx_pin_, tx_pin_);
    auto_calibration_off();
  }

  int read_co2ppm() {
    serial_.write(read_co2_command, MHZ19_REQUEST_LENGTH);
    serial_.flush();

    int i = 0;
    while (serial_.available() <= 0) {
      if (RETRY_COUNT < i) {
        dprintln("Cannot get return value");
        return -1;
      }
      i++;

      delay(1000);
    }

    uint8_t buf[MHZ19_REQUEST_LENGTH] = {0};
    size_t read_size = serial_.readBytes(buf, MHZ19_REQUEST_LENGTH);

#ifdef DEBUG
    dprint("Read Data: ");
    for (int i = 0; i < MHZ19_REQUEST_LENGTH; i++) {
      dprintf("%02x ", buf[i]);
    }
    dprintln("");
#endif

    if (read_size != MHZ19_REQUEST_LENGTH) {
      dprintln("Invalid read data size");
      dprintf("Read data size: %d\n", read_size);

      return -1;
    }

    if (checksum(buf) != buf[8]) {
      dprintln("Invalid checksum");
      dprintf("Checksum calc(%02x) != read(%02x)\n", checksum(buf), buf[8]);
      return -1;
    }

    if (buf[1] == 0x86) {
      return (buf[2] << 8) + buf[3];
    } else {
      dprintln("Invalid opcode");
      return -1;
    }
  }

private:
  HardwareSerial serial_;

  int8_t rx_pin_;
  int8_t tx_pin_;

  uint8_t checksum(uint8_t *packet) {
    uint8_t i = 0;
    uint8_t checksum = 0;
    for (i = 1; i < 8; i++) {
      checksum += packet[i];
    }
    checksum = 0xff - checksum;
    return checksum + 0x01;
  }

  void auto_calibration_off(void) {
    serial_.write(auto_calibration_off_command, MHZ19_REQUEST_LENGTH);
    serial_.flush();
  }
};

extern MHZ19 mhz19;
