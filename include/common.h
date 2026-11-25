#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <U8g2lib.h>
#include <vector>

#include "config.h"
#include "music.h"

extern U8G2& u8g2;
extern MusicPlayer music;
extern Preferences prefs;

// ---------------------------------------------------------------------------
// Botones: press = recién apretado, shortRel = soltado antes del "mantener",
// longPress = se mantuvo apretado `longMs`.
// ---------------------------------------------------------------------------
struct Button {
  uint8_t pin;
  uint16_t longMs;
  bool down = false, longFired = false;
  uint32_t changed = 0, pressedAt = 0;
  bool press = false, shortRel = false, longPress = false;

  Button(uint8_t p, uint16_t l) : pin(p), longMs(l) {}
  void begin() { pinMode(pin, BTN_TO_GND ? INPUT_PULLUP : INPUT_PULLDOWN); }
  void poll(uint32_t now) {
    press = shortRel = longPress = false;
    bool raw = digitalRead(pin) == (BTN_TO_GND ? LOW : HIGH);
    if (raw != down && now - changed > 25) {
      changed = now;
      down = raw;
      if (down) {
        press = true;
        pressedAt = now;
        longFired = false;
      } else if (!longFired) {
        shortRel = true;
      }
    }
    if (down && !longFired && now - pressedAt > longMs) {
      longFired = true;
      longPress = true;
    }
  }
};

extern Button btnL, btnOk, btnR;
extern bool comboFired;  // izquierda + derecha apretados juntos (al soltarlos)
extern bool shakeFired;   // sacudida detectada por el acelerómetro
extern bool motionReady;  // el MPU-6050 respondió al arrancar
void motionBegin();
void motionPoll(uint32_t now);
void motionDrawDebug(uint32_t now);

// ---------------------------------------------------------------------------
// Dibujo
// ---------------------------------------------------------------------------
void drawSprite(int x, int y, const char* const* rows, int h);
void drawSpriteMirror(int x, int y, const char* const* rows, int h);
void drawSpriteDissolve(int x, int y, const char* const* rows, int h, int percent);
void centerText(int y, const char* s);
void drawSparkle(int x, int y, int size);
void drawToast(const char* msg, int y = 26);
std::vector<String> wrapText(const String& text, int width);
String upperUtf8(const String& s);

// Caja de texto que escribe letra por letra, con páginas.
struct TypeBox {
  std::vector<String> lines;
  int perPage = 3, page = 0, typed = 0;
  uint32_t last = 0;

  void set(const String& text, int width, int per) {
    u8g2.setFont(u8g2_font_6x10_tf);
    lines = wrapText(text, width);
    perPage = per;
    page = 0;
    typed = 0;
  }
  int pages() const { return (lines.size() + perPage - 1) / perPage; }
  bool lastPage() const { return page + 1 >= pages(); }
  int pageBytes() const {
    int total = 0;
    for (int i = 0; i < perPage; i++) {
      int li = page * perPage + i;
      if (li < (int)lines.size()) total += lines[li].length();
    }
    return total;
  }
  uint8_t byteAt(int k) const {
    for (int i = 0; i < perPage; i++) {
      int li = page * perPage + i;
      if (li >= (int)lines.size()) break;
      int len = lines[li].length();
      if (k < len) return (uint8_t)lines[li][k];
      k -= len;
    }
    return 0;
  }
  bool full() const { return typed >= pageBytes(); }
  void tick(uint32_t now) {
    if (now - last < 35 || full()) return;
    last = now;
    int total = pageBytes();
    typed++;
    while (typed < total && (byteAt(typed) & 0xC0) == 0x80) typed++;
  }
  // Completa la página, pasa a la siguiente, o devuelve true si ya terminó.
  bool ok() {
    if (!full()) {
      typed = pageBytes();
      return false;
    }
    if (!lastPage()) {
      page++;
      typed = 0;
      return false;
    }
    return true;
  }
  // x < 0 = cada línea centrada
  void draw(int x, int y0, int lineH, uint32_t now) const {
    u8g2.setFont(u8g2_font_6x10_tf);
    int remaining = typed;
    for (int i = 0; i < perPage && remaining > 0; i++) {
      int li = page * perPage + i;
      if (li >= (int)lines.size()) break;
      int n = min(remaining, (int)lines[li].length());
      int lx = x >= 0 ? x : (128 - u8g2.getUTF8Width(lines[li].c_str())) / 2;
      u8g2.drawUTF8(lx, y0 + i * lineH, lines[li].substring(0, n).c_str());
      remaining -= n;
    }
    if (full() && !lastPage() && (now / 300) % 2 == 0) u8g2.drawTriangle(116, 57, 123, 57, 119, 61);
  }
};

// ---------------------------------------------------------------------------
// Modos
// ---------------------------------------------------------------------------
void oracleEnter(uint32_t now);
void oracleUpdate(uint32_t now);
void oracleDraw(uint32_t now);
void oracleWake(uint32_t now);  // prende la pantalla si estaba dormida

// Notificaciones del teléfono (encima de cualquier modo)
void notifyBegin();
bool notifyUpdate(uint32_t now);  // true = se quedó con los botones
void notifyDraw(uint32_t now);
void notifyDrawBadge(uint32_t now);
bool notifyActive();
bool notifyConnected();
bool notifyOpenHistory(uint32_t now);

void dungeonBegin();
void dungeonEnter(uint32_t now);
void dungeonUpdate(uint32_t now);
void dungeonDraw(uint32_t now);
