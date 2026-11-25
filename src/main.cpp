// Oráculo y Mazmorra
// XIAO ESP32-S3 + OLED 128x64 I2C + buzzer pasivo + 3 botones + sensor de inclinación
//
// Siempre arranca en el oráculo: lo agitás y te da un consejo.
// Mantener OK cambia a la mazmorra (y vuelve).

#include <Wire.h>

#include "common.h"
#include "songs.h"

#if OLED_TYPE == 1
static U8G2_SSD1309_128X64_NONAME0_F_HW_I2C display(U8G2_R0, PIN_OLED_RES, PIN_OLED_SCL, PIN_OLED_SDA);
#elif OLED_TYPE == 2
static U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, PIN_OLED_RES, PIN_OLED_SCL, PIN_OLED_SDA);
#else
static U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R0, PIN_OLED_RES, PIN_OLED_SCL, PIN_OLED_SDA);
#endif
U8G2& u8g2 = display;

MusicPlayer music;
Preferences prefs;

Button btnL(PIN_BTN_PREV, 800), btnOk(PIN_BTN_OK, 1200), btnR(PIN_BTN_NEXT, 800);
bool comboFired = false;
bool shakeFired = false;

enum Mode { M_ORACLE, M_DUNGEON };
static Mode mode = M_ORACLE;

static uint32_t bannerUntil = 0;
static const char* bannerText = "";
static uint32_t muteToastUntil = 0;

// ---------- Sensor de inclinación ----------
#if TILT_ENABLED
static bool tiltState = false;
static uint32_t tiltChanged = 0, lastShake = 0;
static uint32_t tiltEdges[8];
static int tiltHead = 0;

static void pollTilt(uint32_t now) {
  bool raw = digitalRead(PIN_TILT) == LOW;
  if (raw == tiltState || now - tiltChanged < 8) return;
  tiltState = raw;
  tiltChanged = now;
  tiltEdges[tiltHead] = now;
  tiltHead = (tiltHead + 1) % 8;
  int recent = 0;
  for (int i = 0; i < 8; i++)
    if (tiltEdges[i] && now - tiltEdges[i] < TILT_WINDOW_MS) recent++;
  if (recent >= TILT_EDGES && now - lastShake > 1500) {
    shakeFired = true;
    lastShake = now;
    memset(tiltEdges, 0, sizeof(tiltEdges));
  }
}
#endif

// ---------- Izquierda + derecha juntos ----------
static void pollCombo() {
  static bool armed = false;
  comboFired = false;
  if (btnL.down && btnR.down) {
    armed = true;
    btnL.longFired = btnR.longFired = true;  // ni giros ni "mantener"
  }
  if (armed) {
    btnL.shortRel = btnR.shortRel = false;
    btnL.longPress = btnR.longPress = false;
    if (!btnL.down && !btnR.down) {
      armed = false;
      comboFired = true;
    }
  }
}

static void showBanner(const char* text, uint32_t now) {
  bannerText = text;
  bannerUntil = now + 900;
}

static void switchMode(uint32_t now) {
  u8g2.setContrast(255);
  music.play(&SONG_MODE);
  if (mode == M_ORACLE) {
    mode = M_DUNGEON;
    dungeonEnter(now);
    showBanner("MAZMORRA", now);
  } else {
    mode = M_ORACLE;
    oracleEnter(now);
    showBanner("ORÁCULO", now);
  }
}

static void drawBanner() {
  u8g2.setDrawColor(0);
  u8g2.drawBox(0, 0, 128, 64);
  u8g2.setDrawColor(1);
  u8g2.drawFrame(2, 2, 124, 60);
  u8g2.drawFrame(5, 5, 118, 54);
  u8g2.setFont(u8g2_font_ncenB14_tf);
  centerText(39, bannerText);
}

static void drawSpeaker(int x, int y, bool muted) {
  u8g2.drawBox(x, y + 3, 3, 4);
  u8g2.drawTriangle(x + 2, y + 5, x + 7, y, x + 7, y + 10);
  if (muted) {
    u8g2.drawLine(x + 10, y + 2, x + 16, y + 8);
    u8g2.drawLine(x + 10, y + 8, x + 16, y + 2);
  } else {
    u8g2.drawCircle(x + 7, y + 5, 4, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_LOWER_RIGHT);
    u8g2.drawCircle(x + 7, y + 5, 7, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_LOWER_RIGHT);
  }
}

static void drawMuteToast() {
  const int w = 84, h = 24, x = (128 - w) / 2, y = 20;
  u8g2.setDrawColor(0);
  u8g2.drawBox(x, y, w, h);
  u8g2.setDrawColor(1);
  u8g2.drawRFrame(x, y, w, h, 3);
  drawSpeaker(x + 6, y + 7, music.muted());
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawUTF8(x + 28, y + 16, music.muted() ? "Silencio" : "Sonido");
}

void setup() {
  Serial.begin(115200);
  Serial.setTxTimeoutMs(0);  // no trabarse si nadie lee el puerto
  btnL.begin();
  btnOk.begin();
  btnR.begin();
#if TILT_ENABLED
  pinMode(PIN_TILT, INPUT_PULLUP);
#endif
  music.begin(PIN_BUZZER);
  prefs.begin("oraculo", false);
  music.setMuted(prefs.getBool("muted", false));
  randomSeed(esp_random());

  u8g2.setBusClock(400000);
  u8g2.begin();
  u8g2.setFontMode(1);

  dungeonBegin();
  uint32_t now = millis();
  oracleEnter(now);
  showBanner("ORÁCULO", now);
  music.play(&SONG_REVEAL);
}

void loop() {
  uint32_t now = millis();
  btnL.poll(now);
  btnOk.poll(now);
  btnR.poll(now);
  pollCombo();
  shakeFired = false;
#if TILT_ENABLED
  pollTilt(now);
#endif

  // Mantener izquierda = silenciar / activar el sonido (se guarda)
  if (btnL.longPress) {
    music.setMuted(!music.muted());
    prefs.putBool("muted", music.muted());
    muteToastUntil = now + 1200;
  }
  // Mantener OK = cambiar entre oráculo y mazmorra
  if (btnOk.longPress) switchMode(now);

  bool banner = (int32_t)(bannerUntil - now) > 0;
  if (!banner) {
    if (mode == M_ORACLE) oracleUpdate(now);
    else dungeonUpdate(now);
  }

  now = millis();
  u8g2.clearBuffer();
  if (banner) drawBanner();
  else if (mode == M_ORACLE) oracleDraw(now);
  else dungeonDraw(now);
  if ((int32_t)(muteToastUntil - now) > 0) drawMuteToast();
  u8g2.sendBuffer();
}
