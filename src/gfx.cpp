#include "common.h"

void drawSprite(int x, int y, const char* const* rows, int h) {
  for (int r = 0; r < h; r++)
    for (int c = 0; rows[r][c]; c++)
      if (rows[r][c] == '#') u8g2.drawPixel(x + c, y + r);
}

void drawSpriteMirror(int x, int y, const char* const* rows, int h) {
  for (int r = 0; r < h; r++) {
    int w = strlen(rows[r]);
    for (int c = 0; c < w; c++)
      if (rows[r][c] == '#') u8g2.drawPixel(x + w - 1 - c, y + r);
  }
}

// Dibuja solo una parte de los pixeles (al azar): sirve para "aparecer" de a poco.
void drawSpriteDissolve(int x, int y, const char* const* rows, int h, int percent) {
  for (int r = 0; r < h; r++)
    for (int c = 0; rows[r][c]; c++)
      if (rows[r][c] == '#' && (int)random(100) < percent) u8g2.drawPixel(x + c, y + r);
}

void centerText(int y, const char* s) {
  u8g2.drawUTF8((128 - u8g2.getUTF8Width(s)) / 2, y, s);
}

void drawSparkle(int x, int y, int size) {
  if (size <= 0) return;
  u8g2.drawPixel(x, y);
  if (size >= 2) {
    u8g2.drawPixel(x - 1, y); u8g2.drawPixel(x + 1, y);
    u8g2.drawPixel(x, y - 1); u8g2.drawPixel(x, y + 1);
  }
  if (size >= 3) {
    u8g2.drawPixel(x - 2, y); u8g2.drawPixel(x + 2, y);
    u8g2.drawPixel(x, y - 2); u8g2.drawPixel(x, y + 2);
  }
}

// Cartelito centrado sobre lo que haya en pantalla.
void drawToast(const char* msg) {
  u8g2.setFont(u8g2_font_6x10_tf);
  int w = u8g2.getUTF8Width(msg) + 10, h = 15;
  if (w > 128) w = 128;
  int x = (128 - w) / 2, y = 26;
  u8g2.setDrawColor(0);
  u8g2.drawBox(x, y, w, h);
  u8g2.setDrawColor(1);
  u8g2.drawRFrame(x, y, w, h, 3);
  u8g2.drawUTF8(x + 5, y + 11, msg);
}

// Corta un texto en líneas que entren en `width` píxeles (usa la fuente actual).
std::vector<String> wrapText(const String& text, int width) {
  std::vector<String> lines;
  String src = text + '\n';
  String para;
  for (unsigned i = 0; i < src.length(); i++) {
    char ch = src[i];
    if (ch != '\n') {
      para += ch;
      continue;
    }
    String line;
    int start = 0;
    while (start <= (int)para.length()) {
      int sp = para.indexOf(' ', start);
      if (sp < 0) sp = para.length();
      String word = para.substring(start, sp);
      String test = line.length() ? line + " " + word : word;
      if (line.length() && u8g2.getUTF8Width(test.c_str()) > width) {
        lines.push_back(line);
        line = word;
      } else {
        line = test;
      }
      start = sp + 1;
    }
    lines.push_back(line);
    para = "";
  }
  return lines;
}

// Mayúsculas, incluyendo á é í ó ú ñ ü (UTF-8 de 2 bytes 0xC3 xx).
String upperUtf8(const String& s) {
  String o = s;
  for (unsigned i = 0; i < o.length(); i++) {
    uint8_t c = o[i];
    if (c >= 'a' && c <= 'z') o.setCharAt(i, c - 32);
    else if (c == 0xC3 && i + 1 < o.length()) {
      uint8_t n = o[i + 1];
      if (n >= 0xA0 && n <= 0xBE && n != 0xB7) o.setCharAt(i + 1, n - 0x20);
      i++;
    }
  }
  return o;
}
