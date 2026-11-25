// Notificaciones del teléfono por Bluetooth (app Chronos en Android).
// Cuando llega una, un hada mensajera trae una carta y se abre encima de
// lo que estés haciendo. OK la cierra, izquierda/derecha recorren las últimas.

#include <ChronosESP32.h>

#include "common.h"
#include "songs.h"
#include "sprites.h"

namespace {

ChronosESP32 watch("Oraculo");

const int HIST = 10;       // cuántas guarda
const int TXT_MAX = 220;   // largo máximo del mensaje (bytes)

struct Msg {
  int icon;
  char app[20];
  char time[6];
  char title[48];
  char text[TXT_MAX];
  bool seen;
};

// Lo que llega desde la tarea de Bluetooth: se copia acá y el loop lo procesa.
struct Raw {
  int icon;
  char app[20], time[6], title[64], text[TXT_MAX + 40];
};
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
Raw inbox[4];
volatile int inHead = 0, inTail = 0;
volatile bool connected = false;
volatile int callState = 0;  // 0 nada, 1 empezó a sonar, 2 cortaron
char callerRaw[48];

Msg hist[HIST];
int histCount = 0;  // hist[0] = la más nueva

enum NState { N_NONE, N_ARRIVE, N_READ, N_CALL };
NState nst = N_NONE;
uint32_t nAt = 0, lastInput = 0;
int cur = 0;          // índice en hist que se está leyendo
bool newWhileOpen = false;
TypeBox box;
char caller[48];

const uint32_t ARRIVE_MS = 1100;
const uint32_t READ_TIMEOUT = 25000;

void copyStr(char* dst, size_t n, const String& s) {
  strncpy(dst, s.c_str(), n - 1);
  dst[n - 1] = 0;
}

void onNotification(Notification n) {
  portENTER_CRITICAL(&mux);
  int next = (inHead + 1) % 4;
  if (next != inTail) {
    Raw& r = inbox[inHead];
    r.icon = n.icon;
    copyStr(r.app, sizeof(r.app), n.app);
    copyStr(r.time, sizeof(r.time), n.time);
    copyStr(r.title, sizeof(r.title), n.title);
    copyStr(r.text, sizeof(r.text), n.message);
    inHead = next;
  }
  portEXIT_CRITICAL(&mux);
}

void onConnection(bool c) { connected = c; }

void onRinger(String who, bool ringing) {
  portENTER_CRITICAL(&mux);
  copyStr(callerRaw, sizeof(callerRaw), who);
  callState = ringing ? 1 : 2;
  portEXIT_CRITICAL(&mux);
}

// Deja solo lo que la fuente de la pantalla puede dibujar: ASCII y Latin-1
// (acentos, ñ, ¿¡). Emojis y otros símbolos se van; los espacios repetidos se juntan.
void sanitize(const char* in, char* out, size_t n) {
  size_t o = 0;
  const uint8_t* p = (const uint8_t*)in;
  while (*p && o + 3 < n) {
    uint8_t c = *p;
    int len = c < 0x80 ? 1 : (c >> 5) == 6 ? 2 : (c >> 4) == 14 ? 3 : (c >> 3) == 30 ? 4 : 1;
    bool keep = false;
    if (len == 1) {
      if (c == '\n') keep = true;
      else if (c >= 0x20 && c < 0x7F) keep = true;
    } else if (len == 2 && (c == 0xC2 || c == 0xC3) && p[1]) {
      keep = !(c == 0xC2 && p[1] < 0xA0);  // sin caracteres de control
    } else if (len == 3 && c == 0xE2 && p[1] == 0x80 && p[2]) {
      // comillas y guiones "lindos" -> ASCII
      char rep = 0;
      if (p[2] == 0x98 || p[2] == 0x99) rep = '\'';
      else if (p[2] == 0x9C || p[2] == 0x9D) rep = '"';
      else if (p[2] == 0x93 || p[2] == 0x94) rep = '-';
      else if (p[2] == 0xA6) rep = '.';
      if (rep) {
        out[o++] = rep;
        if (p[2] == 0xA6 && o + 3 < n) { out[o++] = '.'; out[o++] = '.'; }
      }
    }
    if (keep && !(c == ' ' && (o == 0 || out[o - 1] == ' ' || out[o - 1] == '\n'))) {
      for (int k = 0; k < len && p[k]; k++) out[o++] = p[k];
    }
    for (int k = 0; k < len && *p; k++) p++;
  }
  while (o && (out[o - 1] == ' ' || out[o - 1] == '\n')) o--;
  out[o] = 0;
}

int unreadCount() {
  int u = 0;
  for (int i = 0; i < histCount; i++)
    if (!hist[i].seen) u++;
  return u;
}

int oldestUnread() {
  for (int i = histCount - 1; i >= 0; i--)
    if (!hist[i].seen) return i;
  return -1;
}

void openMsg(int i, uint32_t now, const Song* sound = &SONG_PAGE) {
  if (sound) music.play(sound);
  cur = i;
  hist[i].seen = true;
  String body = hist[i].text[0] ? hist[i].text : "(sin texto)";
  box.set(body, 124, 3);
  nst = N_READ;
  nAt = now;
  lastInput = now;
}

void arrive(uint32_t now) {
  u8g2.setContrast(255);
  music.play(&SONG_LETTER);
  if (nst == N_READ) {
    newWhileOpen = true;  // no interrumpe la que estás leyendo
    return;
  }
  nst = N_ARRIVE;
  nAt = now;
}

void close(uint32_t now) {
  if (nst == N_READ) music.play(&SONG_LETTER_CLOSE);
  nst = N_NONE;
  newWhileOpen = false;
  oracleWake(now);
}

void pullInbox(uint32_t now) {
  while (true) {
    Raw r;
    bool got = false;
    portENTER_CRITICAL(&mux);
    if (inTail != inHead) {
      r = inbox[inTail];
      inTail = (inTail + 1) % 4;
      got = true;
    }
    portEXIT_CRITICAL(&mux);
    if (!got) break;

    if (histCount < HIST) histCount++;
    for (int i = histCount - 1; i > 0; i--) hist[i] = hist[i - 1];
    if (nst == N_READ) cur = min(cur + 1, histCount - 1);
    Msg& m = hist[0];
    m.icon = r.icon;
    sanitize(r.app, m.app, sizeof(m.app));
    strncpy(m.time, r.time, sizeof(m.time) - 1);
    m.time[sizeof(m.time) - 1] = 0;
    sanitize(r.title, m.title, sizeof(m.title));
    sanitize(r.text, m.text, sizeof(m.text));
    m.seen = false;
    arrive(now);
  }

  int cs;
  portENTER_CRITICAL(&mux);
  cs = callState;
  callState = 0;
  if (cs == 1) strcpy(caller, callerRaw);
  portEXIT_CRITICAL(&mux);
  if (cs == 1) {
    char tmp[48];
    sanitize(caller, tmp, sizeof(tmp));
    strcpy(caller, tmp[0] ? tmp : "Desconocido");
    u8g2.setContrast(255);
    nst = N_CALL;
    nAt = now;
  } else if (cs == 2 && nst == N_CALL) {
    music.stop();
    close(now);
  }
}

// Recorta un texto para que entre en `w` píxeles, con "..." al final.
String fit(const char* s, int w) {
  String t = s;
  if (u8g2.getUTF8Width(t.c_str()) <= w) return t;
  while (t.length() && u8g2.getUTF8Width((t + "...").c_str()) > w) {
    int cut = t.length() - 1;
    while (cut > 0 && ((uint8_t)t[cut] & 0xC0) == 0x80) cut--;  // no partir un acento
    t.remove(cut);
  }
  return t + "...";
}

// Nombre de la app según el código que manda Chronos. Si Chronos no la
// reconoce manda 3 (genérico): en ese caso solo dice "CARTA".
const char* appLabel(int icon) {
  switch (icon) {
    case 0x04: return "CORREO";
    case 0x08: return "SKYPE";
    case 0x0A: return "WHATSAPP";
    case 0x0B: return "GMAIL";
    case 0x0E: return "LINE";
    case 0x0F: return "X";
    case 0x10: return "FACEBOOK";
    case 0x11: return "MESSENGER";
    case 0x12: return "INSTAGRAM";
    case 0x16: return "VIBER";
    case 0x18: return "TELEGRAM";
    case 0x20: return "WHATSAPP BUSINESS";
    case 0xC0: return "CHRONOS";
    default:   return "CARTA";
  }
}

void drawFairy(int x, int y, uint32_t now) {
  drawSprite(x, y, (now / 90) % 2 ? SPR_FAIRY_A : SPR_FAIRY_B, 8);
}

}  // namespace

void notifyBegin() {
  watch.setNotificationCallback(onNotification);
  watch.setConnectionCallback(onConnection);
  watch.setRingerCallback(onRinger);
  watch.begin();
}

bool notifyActive() { return nst != N_NONE; }
bool notifyConnected() { return connected; }

// Devuelve true si la notificación se quedó con los botones este cuadro.
bool notifyUpdate(uint32_t now) {
  watch.loop();
  pullInbox(now);

  bool ok = btnOk.shortRel, left = btnL.shortRel, right = btnR.shortRel;
  if (ok || left || right) lastInput = now;

  switch (nst) {
    case N_NONE:
      return false;

    case N_ARRIVE:
      if (now - nAt > ARRIVE_MS || ok) {
        int i = oldestUnread();
        if (i < 0) close(now);
        else openMsg(i, now, &SONG_LETTER_OPEN);
      }
      return true;

    case N_READ:
      box.tick(now);
      if (ok) {
        if (box.ok()) {
          int i = oldestUnread();
          if (i >= 0) {
            openMsg(i, now, &SONG_LETTER_OPEN);
          } else {
            close(now);
          }
        }
      } else if (left && cur + 1 < histCount) {
        openMsg(cur + 1, now);  // más vieja
      } else if (right && cur > 0) {
        openMsg(cur - 1, now);  // más nueva
      } else if (now - lastInput > READ_TIMEOUT) {
        close(now);
      }
      return true;

    case N_CALL:
      if (!music.playing()) music.play(&SONG_CALL);
      if (ok || left || right) {
        music.stop();
        close(now);
      }
      return true;
  }
  return false;
}

// Abre la última notificación (para volver a leerlas). false si no hay ninguna.
bool notifyOpenHistory(uint32_t now) {
  if (!histCount || nst != N_NONE) return false;
  u8g2.setContrast(255);
  int i = oldestUnread();
  openMsg(i >= 0 ? i : 0, now, &SONG_LETTER_OPEN);
  return true;
}

void notifyDraw(uint32_t now) {
  uint32_t st = now - nAt;
  switch (nst) {
    case N_NONE:
      break;

    case N_ARRIVE: {
      // El hada cruza volando con la carta colgando y deja chispas.
      float t = min(1.0f, st / 700.0f);
      int x = -12 + (int)(t * 70);
      int y = 18 + (int)(4 * sinf(st / 90.0f));
      for (int k = 1; k <= 4; k++) {
        int tx = x - k * 9, ty = 22 + (int)(3 * sinf((st - k * 80) / 90.0f));
        if (tx > 0 && (now / 120 + k) % 2) drawSparkle(tx, ty, 1 + k % 2);
      }
      drawFairy(x, y, now);
      u8g2.drawLine(x + 4, y + 8, x + 4, y + 11);
      drawSprite(x - 2, y + 12, SPR_LETTER, 9);
      if (st > 300) {
        u8g2.setFont(u8g2_font_6x10_tf);
        centerText(56, "¡TENÉS UNA CARTA!");
      }
      break;
    }

    case N_READ: {
      Msg& m = hist[cur];
      // Pergamino: borde doble con esquinas enrolladas
      u8g2.drawFrame(0, 0, 128, 64);
      u8g2.drawBox(0, 0, 128, 11);
      u8g2.setDrawColor(0);
      u8g2.setFont(u8g2_font_5x7_tf);
      u8g2.drawUTF8(3, 8, fit(appLabel(m.icon), 90).c_str());
      int tw = u8g2.getStrWidth(m.time);
      u8g2.drawStr(125 - tw, 8, m.time);
      u8g2.setDrawColor(1);

      u8g2.setFont(u8g2_font_6x10_tf);
      String title = fit(m.title[0] ? m.title : m.app, 122);
      u8g2.drawUTF8(3, 21, title.c_str());
      u8g2.drawHLine(3, 23, min(122, (int)u8g2.getUTF8Width(title.c_str())));

      box.draw(3, 33, 10, now);

      // Pie: posición en el historial y avisos
      u8g2.setFont(u8g2_font_4x6_tf);
      char pos[12];
      snprintf(pos, sizeof(pos), "%d/%d", cur + 1, histCount);
      u8g2.drawStr(3, 62, pos);
      if (cur + 1 < histCount) u8g2.drawTriangle(24, 59, 27, 56, 27, 62);  // hay más viejas
      if (cur > 0) u8g2.drawTriangle(34, 56, 34, 62, 37, 59);              // hay más nuevas
      if (newWhileOpen && (now / 400) % 2) u8g2.drawStr(46, 62, "+ NUEVA");
      break;
    }

    case N_CALL: {
      bool f = (now / 250) % 2;
      u8g2.drawFrame(0, 0, 128, 64);
      if (f) u8g2.drawFrame(2, 2, 124, 60);
      int bob = (int)(3 * sinf(st / 80.0f));
      drawFairy(14, 14 + bob, now);
      drawFairy(105, 14 - bob, now);
      u8g2.setFont(u8g2_font_6x10_tf);
      centerText(20, "LLAMADA");
      u8g2.setFont(u8g2_font_ncenB10_tf);
      String who = fit(caller, 118);
      centerText(42, who.c_str());
      u8g2.setFont(u8g2_font_4x6_tf);
      centerText(57, "cualquier boton: silenciar");
      break;
    }
  }
}

// Sobrecito con la cantidad sin leer (abajo a la izquierda, arriba del
// texto de ayuda, para no tapar la respuesta del oráculo).
void notifyDrawBadge(uint32_t now) {
  int u = unreadCount();
  if (u > 0) {
    int x = 2, y = 45;
    u8g2.setDrawColor(0);
    u8g2.drawBox(x - 1, y - 1, 22, 10);
    u8g2.setDrawColor(1);
    u8g2.drawFrame(x, y, 11, 8);
    u8g2.drawLine(x, y, x + 5, y + 4);
    u8g2.drawLine(x + 10, y, x + 5, y + 4);
    u8g2.setFont(u8g2_font_4x6_tf);
    char b[4];
    snprintf(b, sizeof(b), "%d", u);
    u8g2.drawStr(x + 13, y + 7, b);
  }
  (void)now;
}
