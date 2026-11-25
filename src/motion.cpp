// Acelerómetro MPU-6050 en el mismo bus I2C que la pantalla.
// Detecta "sacudidas": varios picos fuertes de aceleración en poco tiempo.
// Si no lo encuentra al arrancar, lo vuelve a buscar cada 2 segundos.

#include <Wire.h>

#include "common.h"

bool motionReady = false;

namespace {

uint8_t addr = 0, who = 0;
uint32_t lastRead = 0, lastPeak = 0, lastShake = 0, lastProbe = 0, lastErr = 0;
uint32_t peaks[8];
int peakHead = 0, shakes = 0, recentPeaks = 0;
float gNow = 0, peakShow = 0;
uint32_t peakShowAt = 0;

bool writeReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

bool readRegs(uint8_t reg, uint8_t* buf, uint8_t n) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(addr, n) != n) return false;
  for (uint8_t i = 0; i < n; i++) buf[i] = Wire.read();
  return true;
}

bool tryStart() {
  for (uint8_t a : {(uint8_t)0x68, (uint8_t)0x69}) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() != 0) continue;
    addr = a;
    readRegs(0x75, &who, 1);  // WHO_AM_I (0x68 en el MPU-6050; los clones varían)
    return writeReg(0x6B, 0x00)     // despertar
           && writeReg(0x1A, 0x03)  // filtro pasa-bajos ~44 Hz
           && writeReg(0x1C, 0x10); // ±8 g -> 4096 por g
  }
  addr = 0;
  return false;
}

}  // namespace

void motionBegin() {
  motionReady = tryStart();
  lastProbe = millis();
}

void motionPoll(uint32_t now) {
  if (!motionReady) {
    if (now - lastProbe > 2000) {
      lastProbe = now;
      motionReady = tryStart();
    }
    return;
  }
  if (now - lastRead < 10) return;
  lastRead = now;

  uint8_t b[6];
  if (!readRegs(0x3B, b, 6)) {
    lastErr = now;
    return;
  }
  int16_t ax = (b[0] << 8) | b[1];
  int16_t ay = (b[2] << 8) | b[3];
  int16_t az = (b[4] << 8) | b[5];

  // Quieto mide ~1 g (la gravedad), en cualquier posición.
  gNow = sqrtf((float)ax * ax + (float)ay * ay + (float)az * az) / 4096.0f;
  float dev = fabsf(gNow - 1.0f);
  if (dev > peakShow || now - peakShowAt > 1500) {
    peakShow = dev;
    peakShowAt = now;
  }

  if (dev < SHAKE_G || now - lastPeak < 80) return;
  lastPeak = now;
  peaks[peakHead] = now;
  peakHead = (peakHead + 1) % 8;
  recentPeaks = 0;
  for (int i = 0; i < 8; i++)
    if (peaks[i] && now - peaks[i] < SHAKE_WINDOW_MS) recentPeaks++;
  if (recentPeaks >= SHAKE_PEAKS && now - lastShake > 1500) {
    shakeFired = true;
    shakes++;
    lastShake = now;
    memset(peaks, 0, sizeof(peaks));
  }
}

// Renglón de diagnóstico arriba de todo (solo con SHAKE_DEBUG 1).
void motionDrawDebug(uint32_t now) {
  char buf[40];
  if (!motionReady) {
    snprintf(buf, sizeof(buf), "SIN SENSOR (busca 0x68/69)");
  } else {
    snprintf(buf, sizeof(buf), "%02X/%02X g%.2f pk%.2f n%d%s", addr, who, gNow, peakShow, shakes,
             now - lastErr < 500 ? " ERR" : "");
  }
  u8g2.setFont(u8g2_font_4x6_tf);
  int w = u8g2.getStrWidth(buf) + 2;
  u8g2.setDrawColor(0);
  u8g2.drawBox(0, 0, w, 7);
  u8g2.setDrawColor(1);
  u8g2.drawStr(1, 6, buf);
}
