#pragma once

#include <HTTPClient.h>
#include <M5Core2.h>
#include <WiFi.h>

#include "local_time.h"
#include "macro.h"
#include "util.h"
#include "mutex.h"

const char *ca = R"(-----BEGIN CERTIFICATE-----
MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD
QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB
CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97
nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt
43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P
T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4
gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO
BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR
TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw
DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr
hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg
06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF
PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls
YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk
CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=
-----END CERTIFICATE-----)";

#define URL_LEN 74

class Satellite {
 public:
  Satellite() {
    is_active_ = false;
    image_ptr_ = nullptr;
    image_size_ = 0;
    image_time_ = 0;
  };
  ~Satellite(){};

  void begin() {
    http_.setReuse(true);
  }

  bool need_fetch() {
    int new_time = image_time();

    if (new_time < 0) {
      dprintln("Can't get local time");
      return false;
    }

    if (image_time_ == new_time) {
      dprintf("exists image %04d.jpg\n", image_time_);
      return false;
    }

    return true;
  }

  int fetch() {
    int time = image_time();
    char url[URL_LEN] = {0};

    image_url(time, url);

    http_.begin(url, ca);

    dprintf("[GET] %s\n", url);
    int status_code = http_.GET();

    int ret = -1;
    if (0 < status_code) {
      dprintf("HTTP GET %d\n", status_code);

      if (status_code == HTTP_CODE_OK) {

        image_download(&http_, &image_ptr_, &image_size_);

        image_time_ = time;

        ret = 0;
      }
    } else {
      dprintf("HTTP GET ERR: %s\n", http_.errorToString(status_code).c_str());
    }

    http_.end();
    return ret;
  }

  const uint8_t *image_ptr() {
    return image_ptr_;
  }

  size_t image_size() {
    return image_size_;
  }

 private:
  HTTPClient http_;
  bool is_active_;

  int image_time_;
  uint8_t *image_ptr_;
  size_t image_size_;

  int image_time() {
    struct tm tm = localtime();

    time_t now = mktime(&tm);
    time(&now);

    // ひまわりの衛星画像が準備されるまで20分近くかかるため30分ほど引いておく
    now = ((now - 1800) / 600) * 600;

    gmtime_r(&now, &tm);

    return (tm.tm_hour * 100) + tm.tm_min;
  }

  void image_url(int time, char *url) {
    snprintf(url,
             URL_LEN,
             "https://www.data.jma.go.jp/mscweb/data/himawari/img/jpn/"
             "jpn_b13_%04d.jpg",
             time);
  }

  void image_download(HTTPClient *http,
                      uint8_t **image_ptr,
                      size_t *image_size) {
    int data_size = http->getSize();
    uint8_t *ptr = *image_ptr;

    dprintf("Body [%d]\n", data_size);

    ptr = (uint8_t *) ps_realloc(ptr, data_size);
    memset(ptr, 0, data_size);

    WiFiClient *stream = http->getStreamPtr();

    int read_len_sum = 0;
    char buf[32] = {0};
    while (http->connected() && read_len_sum < data_size) {
      int available_size = stream->available();

      if (0 < available_size) {
        int read_len = stream->readBytes((uint8_t *) (ptr + read_len_sum),
                                         available_size);

        read_len_sum += read_len;

        dprintf("HTTP read: %d/%d Bytes\n", read_len_sum, data_size);

        if (mutex && xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {

          m5.lcd.setTextSize(2);
          m5.lcd.setTextColor(WHITE, BLACK);
          snprintf(buf, 32, "%d/%d Bytes\n", read_len_sum, data_size);
          draw_center_center_string(buf);

          xSemaphoreGive(mutex);
        } else {
          dprintln(
              "fetch_and_save_himawari_real_time_image: mutex == NULL or "
              "xSemaphoreTake is failure");
        }
      }
    }

    dprintln("HTTP read done");

    *image_ptr = ptr;
    *image_size = data_size;
  }
};
