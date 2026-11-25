#pragma once
#include <Arduino.h>
#include "songs.h"

inline uint16_t midiToFreq(uint8_t m) {
  return (uint16_t)(440.0f * powf(2.0f, (m - 69) / 12.0f) + 0.5f);
}

// Reproductor que corre en su propia tarea de FreeRTOS (core 0), así la
// música no se traba mientras se dibuja.
class MusicPlayer {
 public:
  void begin(uint8_t pin) {
    pin_ = pin;
    ledcSetup(CH, 2000, 10);
    ledcAttachPin(pin_, CH);
    ledcWrite(CH, 0);
    xTaskCreatePinnedToCore(taskFn, "music", 4096, this, 2, nullptr, 0);
  }

  // Toca `s` y, cuando termina, encadena `then` (opcional).
  void play(const Song* s, const Song* then = nullptr) {
    reqNext_ = then;
    req_ = s;
    reqFlag_ = true;
  }
  void stop() { play(nullptr); }

  // Volumen 0 (silencio) a 3 (alto). El buzzer suena más bajo cuanto más
  // corto es el pulso que le mandamos.
  static const uint8_t VOL_MAX = 3;
  void setVolume(uint8_t v) {
    vol_ = v > VOL_MAX ? VOL_MAX : v;
    muteChanged_ = true;
  }
  uint8_t volume() const { return vol_; }
  bool muted() const { return vol_ == 0; }
  bool playing() const { return reqFlag_ || cur_ != nullptr; }

 private:
  static void taskFn(void* arg) {
    auto* self = static_cast<MusicPlayer*>(arg);
    for (;;) {
      self->update();
      vTaskDelay(pdMS_TO_TICKS(1));
    }
  }

  static const uint8_t CH = 4;  // canal PWM del buzzer
  uint16_t duty() const {
    static const uint16_t D[] = {0, 10, 60, 512};  // de 1023
    return D[vol_];
  }

  void silence() {
    if (toneOn_) ledcWrite(CH, 0);
    toneOn_ = false;
    sounding_ = false;
  }

  void start(const Song* s, const Song* then, uint32_t now) {
    silence();
    next_ = then;
    idx_ = 0;
    noteEnd_ = now;
    soundEnd_ = now;
    cur_ = s;
  }

  void update() {
    uint32_t now = millis();
    if (muteChanged_) {
      muteChanged_ = false;
      if (toneOn_) ledcWrite(CH, duty());  // aplica el volumen nuevo al instante
    }
    if (reqFlag_) {
      reqFlag_ = false;
      start(req_, reqNext_, now);
    }
    const Song* s = cur_;
    if (!s) return;

    if (sounding_ && (int32_t)(now - soundEnd_) >= 0) silence();
    if ((int32_t)(now - noteEnd_) < 0) return;

    if (idx_ >= s->count) {
      const Song* n = next_;
      silence();
      cur_ = nullptr;
      if (n) start(n, nullptr, now);
      return;
    }

    const Note& n = s->notes[idx_++];
    uint32_t dur = (uint32_t)n.ticks * s->msPerTick;
    noteEnd_ += dur;
    if (n.midi) {
      uint32_t gap = dur > 80 ? 18 : (dur > 20 ? 4 : 0);
      if (vol_) {
        ledcChangeFrequency(CH, midiToFreq(n.midi), 10);
        ledcWrite(CH, duty());
        toneOn_ = true;
      }
      sounding_ = true;
      soundEnd_ = noteEnd_ - gap;
    } else {
      silence();
    }
  }

  uint8_t pin_ = 0;
  volatile bool reqFlag_ = false;
  const Song* volatile req_ = nullptr;
  const Song* volatile reqNext_ = nullptr;
  const Song* volatile cur_ = nullptr;
  const Song* next_ = nullptr;
  uint32_t noteEnd_ = 0, soundEnd_ = 0;
  uint16_t idx_ = 0;
  volatile uint8_t vol_ = VOL_MAX;
  volatile bool muteChanged_ = false;
  bool toneOn_ = false;
  volatile bool sounding_ = false;
};
