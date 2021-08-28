#pragma once

#include <HTTPClient.h>
#include <M5Core2.h>
#include <WiFi.h>

#include "local_time.h"
#include "macro.h"

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

HTTPClient http;
void init_http() {
  http.setReuse(true);
  return;
}

const char himawari_url[73] =
    "https://www.data.jma.go.jp/mscweb/data/himawari/img/%s/%s_%s_%d.jpg";

int current_himawari_time = 0;

struct image {
  size_t len;
  uint8_t *ptr;
  int time;
};

struct image satellite_image = {};

int fetch_and_save_himawari_real_time_image() {
  String area = "jpn";
  String band = "b13";

  char time[4] = {0};
  char url[74] = {0};

  struct tm tm = local_time.timeinfo();
  tm.tm_min = (tm.tm_min / 10) * 10;

  int new_himawari_time = (tm.tm_hour * 100) + tm.tm_min;

  if (current_himawari_time == new_himawari_time) {
    dprintf("exists image %04d.jpg\n", current_himawari_time);
    return 1;
  }

  strftime(time, 5, "%0H%0M", &tm);

  snprintf(
      url, 74,
      "https://www.data.jma.go.jp/mscweb/data/himawari/img/%s/%s_%s_%s.jpg",
      area.c_str(), area.c_str(), band.c_str(), time);

  dprintf("[GET] %s\n", url);

  http.begin(url, ca);

  int status_code = http.GET();

  int ret = -1;

  if (0 < status_code) {
    dprintf("HTTP GET %d\n", status_code);

    if (status_code == HTTP_CODE_OK) {
      current_himawari_time = new_himawari_time;

      int len = http.getSize();
      dprintf("Body [%d]\n", len);

      satellite_image.ptr = (uint8_t *)ps_realloc(satellite_image.ptr, len);
      satellite_image.len = len;
      memset(satellite_image.ptr, 0, len);

      multi_heap_info_t info;

      heap_caps_get_info(&info, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

      dprintf("MALLOC free size %d\n", info.total_free_bytes);
      dprintf("MALLOC alloc size %d\n", info.total_allocated_bytes);

      WiFiClient *stream = http.getStreamPtr();
      int read_len = 0;
      while (http.connected() && read_len < len) {
        int sz = stream->available();
        if (0 < sz) {
          int l = stream->readBytes((uint8_t *)(satellite_image.ptr + read_len),
                                    sz);
          read_len += l;
          dprintf("HTTP read: %d/%d\n", read_len, len);
        }
      }

      dprintln("HTTP read done");

      ret = 0;
    }
  } else {
    dprintf("HTTP GET ERR: %s\n", http.errorToString(status_code).c_str());
  }

  http.end();
  return ret;
}
