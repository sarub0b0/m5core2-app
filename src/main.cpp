#if !defined(WIFI_SSID) || !defined(WIFI_PASSWORD)
#error Require defined WIFI_SSID, WIFI_PASSWORD
#endif

#define str(x) xstr(x)
#define xstr(x) #x

#include <M5Core2.h>
#include <WiFi.h>

#include "co2.h"
#include "himawari.h"
#include "local_time.h"
#include "macro.h"

MHZ19 mhz19(13, 14, 2);

void draw_center_center_string(char *str) {
  int sw = m5.lcd.textWidth(str);
  int sh = m5.lcd.fontHeight();

  int lcd_center_x = m5.lcd.width() / 2;
  int lcd_center_y = m5.lcd.height() / 2;

  m5.lcd.drawString(str, lcd_center_x - (sw / 2), lcd_center_y - (sh / 2));
}

class Render {
 public:
  Render(){};
  virtual ~Render() {
  }

  virtual void render(bool redraw){};
};

class Apps {
 public:
  Apps() {
    index_ = 0;
    need_clear_ = false;
  }
  ~Apps() {
  }

  void add(Render *r) {
    renders_.push_back(r);
  }

  void render() {

    if (need_clear_) {
      lcd_clear();
    }

    renders_[index_]->render(need_clear_);

    need_clear_ = false;
  }

  void next() {
    if (index_ + 1 < renders_.size()) {
      index_++;
      need_clear_ = true;
    }
  }

  void prev() {
    if (0 <= index_ - 1) {
      index_--;
      need_clear_ = true;
    }
  }

 private:
  std::vector<Render *> renders_;
  int index_;

  bool need_clear_;

  void lcd_clear() {
    m5.lcd.clear();
  }
};

class RenderCO2 : public Render {
 public:
  static int co2ppm;

  RenderCO2() {
    prev_ppm_ = -1;
  }

  void render(bool redraw) {
    render_();
  }

 private:
  int prev_ppm_;

  void render_() {
    // diff check
    int ppm = co2ppm;

    if (prev_ppm_ == ppm) {
      return;
    }
    dprintf("co2ppm update: %d\n", ppm);
    prev_ppm_ = ppm;

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
};

int RenderCO2::co2ppm = 0;

// void render_co2ppm() {
//   // diff check
//   int ppm = co2ppm;

//   if (prev_co2ppm == ppm) {
//     return;
//   }
//   dprintf("co2ppm update: %d\n", ppm);
//   prev_co2ppm = ppm;

//   if (0 <= ppm && ppm <= 1000) {
//     m5.lcd.setTextColor(WHITE, BLACK);
//   } else if (1000 < ppm && ppm <= 1500) {
//     m5.lcd.setTextColor(ORANGE, BLACK);
//   } else {
//     m5.lcd.setTextColor(RED, BLACK);
//   }

//   m5.lcd.setTextSize(3);
//   char str[9] = {0};
//   snprintf(str, 9, "%4d ppm", ppm);
//   draw_center_center_string(str);
// }

void init_wifi() {
  m5.lcd.setTextSize(2);

  WiFi.begin(str(WIFI_SSID), str(WIFI_PASSWORD));

  m5.lcd.print("\nWaiting connect to WiFi...");

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    m5.lcd.print(".");
    delay(1000);
    i++;
    if (i == 10) {
      m5.lcd.print("\nConnect failed");
    }
  }

  m5.lcd.println("\nWiFi connected");

  m5.lcd.printf("SSID: %s\n", WiFi.SSID().c_str());
  m5.lcd.printf("IP address: %s\n", WiFi.localIP().toString().c_str());

  delay(1000);

  m5.lcd.fillScreen(BLACK);
}

class RenderHimawari : public Render {
 public:
  RenderHimawari() {
  }

  void render(bool redraw) {
    int ret = fetch_and_save_himawari_real_time_image(redraw);

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
  }

 private:
};

// void render_himawari() {
//   int ret = fetch_and_save_himawari_real_time_image();

//   if (ret == 0) {
//     m5.lcd.drawJpg((const uint8_t *) satellite_image.ptr,
//                    satellite_image.len,
//                    0,
//                    0,
//                    m5.lcd.width(),
//                    m5.lcd.height(),
//                    50,
//                    0,
//                    JPEG_DIV_2);
//   }
// }

void task_co2ppm(void *arg) {
  while (true) {
    RenderCO2::co2ppm = mhz19.read_co2ppm();
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

Apps apps;
void task_render(void *arg) {
  while (true) {
    apps.render();
    delay(1000);
  }
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
  xTaskCreatePinnedToCore(task_render, "Render", 8192, NULL, 1, NULL, 1);

  timer = timerBegin(0, 80, true);

  timerAttachInterrupt(timer, on_timer_local_time, true);
  timerAlarmWrite(timer, 500 * 1000, true);
  timerAlarmEnable(timer);

  dprintf("LCD size %d:%d\n", m5.lcd.width(), m5.lcd.height());

  RenderCO2 *render_co2 = new RenderCO2;
  RenderHimawari *render_himawari = new RenderHimawari;

  apps.add(render_co2);
  apps.add(render_himawari);
}

/*----------------------------------------------------------
    MH-Z19 CO2 sensor  loop
  ----------------------------------------------------------*/
void loop() {
  m5.update();

  if (m5.BtnA.wasReleased()) {
    dprintln("Previous App");
    apps.prev();
  }

  if (m5.BtnB.wasReleased()) {
    dprintln("Press B button");
  }

  if (m5.BtnC.wasReleased()) {
    dprintln("Next App");
    apps.next();
  }
}
