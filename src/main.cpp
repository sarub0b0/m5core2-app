#if !defined(WIFI_SSID) || !defined(WIFI_PASSWORD)
#error Require defined WIFI_SSID, WIFI_PASSWORD
#endif

#define str(x) xstr(x)
#define xstr(x) #x

#include <HTTPClient.h>
#include <M5Core2.h>
#include <WiFi.h>

#include "co2.h"
#include "local_time.h"
#include "macro.h"

MHZ19 mhz19(13, 14, 2);

LocalTime local_time;

void draw_center_center_string(char *str) {
  int sw = m5.lcd.textWidth(str);
  int sh = m5.lcd.fontHeight();

  int lcd_center_x = m5.lcd.width() / 2;
  int lcd_center_y = m5.lcd.height() / 2;

  m5.lcd.drawString(str, lcd_center_x - (sw / 2), lcd_center_y - (sh / 2));
}

int prev_co2ppm = 0;
int co2ppm = 0;
void render_co2ppm() {
  // diff check
  int ppm = co2ppm;

  if (prev_co2ppm == ppm) {
    return;
  }
  dprintf("co2ppm update: %d\n", ppm);
  prev_co2ppm = ppm;

  if (0 <= ppm && ppm <= 1000) {
    m5.lcd.setTextColor(WHITE, BLACK);
  } else if (1000 < ppm && ppm <= 1500) {
    m5.lcd.setTextColor(ORANGE, BLACK);
  } else {
    m5.lcd.setTextColor(RED, BLACK);
  }

  m5.lcd.setTextSize(3);
  char str[9] = {0};
  snprintf(str, 9, "%4d ppm", ppm);
  draw_center_center_string(str);
}

void init_wifi() {
  m5.lcd.setTextSize(2);

  WiFi.begin(str(WIFI_SSID), str(WIFI_PASSWORD));

  m5.lcd.print("\nWaiting connect to WiFi...");

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    m5.lcd.print(".");
    delay(1000);
    i++;
    if (i == 10) m5.lcd.print("\nConnect failed");
  }

  m5.lcd.println("\nWiFi connected");

  m5.lcd.printf("SSID: %s\n", WiFi.SSID().c_str());
  m5.lcd.printf("IP address: %s\n", WiFi.localIP().toString().c_str());

  delay(1000);

  m5.lcd.fillScreen(BLACK);
}

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

const char himawari_url[73] =
    "https://www.data.jma.go.jp/mscweb/data/himawari/img/%s/%s_%s_%d.jpg";

HTTPClient http;
void init_http() {
  http.setReuse(true);
  return;
}

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
      url,
      74,
      "https://www.data.jma.go.jp/mscweb/data/himawari/img/%s/%s_%s_%s.jpg",
      area.c_str(),
      area.c_str(),
      band.c_str(),
      time);

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

      satellite_image.ptr = (uint8_t *) ps_realloc(satellite_image.ptr, len);
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
          int l = stream->readBytes(
              (uint8_t *) (satellite_image.ptr + read_len), sz);
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

void render_himawari() {
  int ret = fetch_and_save_himawari_real_time_image();

  if (ret == 0) {
    m5.lcd.drawJpg((const uint8_t *) satellite_image.ptr,
                   satellite_image.len,
                   0,
                   0,
                   m5.lcd.width(),
                   m5.lcd.height(),
                   50,
                   0,
                   JPEG_DIV_2);
  }

  delay(1000);
}

void task_co2ppm(void *arg) {
  while (true) {
    co2ppm = mhz19.read_co2ppm();
    delay(2000);
  }
}

void IRAM_ATTR on_timer_local_time() {
  m5.lcd.setCursor(0, 0);
  m5.lcd.setTextSize(2);
  m5.lcd.setTextColor(WHITE, BLACK);
  struct tm tm = local_time.timeinfo();
  m5.lcd.println(&tm, SHORT_DATETIME_FORMAT);
}

hw_timer_t *timer = NULL;

/*----------------------------------------------------------
    MH-Z19 CO2 sensor  setup
  ----------------------------------------------------------*/
void setup() {
  m5.begin();

  init_wifi();

  mhz19.begin();

  init_http();

  local_time.begin();

  xTaskCreatePinnedToCore(task_co2ppm, "CO2 ppm", 8192, NULL, 1, NULL, 1);

  timer = timerBegin(0, 80, true);

  timerAttachInterrupt(timer, on_timer_local_time, true);
  timerAlarmWrite(timer, 500 * 1000, true);
  timerAlarmEnable(timer);

  dprintf("LCD size %d:%d\n", m5.lcd.width(), m5.lcd.height());
}

/*----------------------------------------------------------
    MH-Z19 CO2 sensor  loop
  ----------------------------------------------------------*/
void loop() {
  render_co2ppm();

  //   render_time();
  //   render_himawari();
  //
  delay(1000);
}
