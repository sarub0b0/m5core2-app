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
  struct image {
    size_t size;
    uint8_t *ptr;
    int time;
  };

 public:
  Satellite() {
    is_active_ = false;
    image_ = {0, 0, 0};
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

    if (image_.time == new_time) {
      dprintf("exists image %04d.jpg\n", image_.time);
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

        int len = http_.getSize();
        dprintf("Body [%d]\n", len);

        image_.ptr = (uint8_t *) ps_realloc(image_.ptr, len);
        image_.size = len;
        memset(image_.ptr, 0, len);

        WiFiClient *stream = http_.getStreamPtr();
        int read_len = 0;

        char buf[128] = {0};
        while (http_.connected() && read_len < len) {
          int sz = stream->available();
          if (0 < sz) {
            int l =
                stream->readBytes((uint8_t *) (image_.ptr + read_len), sz);

            read_len += l;

            dprintf("HTTP read: %d/%d\n", read_len, len);

            if (mutex && xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
              m5.lcd.setTextSize(2);
              snprintf(buf, 128, "HTTP read: %d/%d\n", read_len, len);
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
        image_.time = time;

        ret = 0;
      }
    } else {
      dprintf("HTTP GET ERR: %s\n", http_.errorToString(status_code).c_str());
    }

    http_.end();
    return ret;
  }

  const uint8_t *image_ptr() {
    return image_.ptr;
  }

  size_t image_size() {
    return image_.size;
  }

 private:
  HTTPClient http_;

  bool is_active_;

  struct image image_;

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
};
