#pragma once
#include <stdint.h>

// Una nota: número MIDI (0 = silencio) + duración en "ticks" de la canción.
struct Note {
  uint8_t midi;
  uint8_t ticks;
};

struct Song {
  const Note* notes;
  uint16_t count;
  uint16_t msPerTick;
};

// Nombres de notas en minúscula (para no chocar con A0..A5 de Arduino)
namespace nt {
enum : uint8_t {
  r = 0,
  c4 = 60, cs4, d4, ds4, e4, f4, fs4, g4, gs4, a4, as4, b4,
  c5, cs5, d5, ds5, e5, f5, fs5, g5, gs5, a5, as5, b5,
  c6, cs6, d6, ds6, e6, f6, fs6, g6
};
}

#define SONG(var, ms, ...)                                     \
  static const Note var##_notes[] = {__VA_ARGS__};             \
  static const Song var = {var##_notes, sizeof(var##_notes) / sizeof(Note), ms};

using namespace nt;

// Todas las melodías son propias.

// ------------------------------ Oráculo ------------------------------
// Temblor que sube mientras el oráculo "piensa"
SONG(SONG_RUMBLE, 34,
  {c4,1},{cs4,1},{c4,1},{cs4,1},{d4,1},{cs4,1},{d4,1},{ds4,1},
  {d4,1},{ds4,1},{e4,1},{ds4,1},{e4,1},{f4,1},{e4,1},{f4,1},
  {fs4,1},{f4,1},{fs4,1},{g4,1},{fs4,1},{g4,1},{gs4,1},{g4,1},
  {a4,1},{gs4,1},{a4,1},{as4,1},{b4,1},{c5,1},{cs5,1},{d5,1},{r,6})
// La respuesta aparece
SONG(SONG_REVEAL, 70,
  {d5,1},{a5,1},{d6,1},{f6,3},{e6,1},{d6,1},{a5,4})
SONG(SONG_BLIP, 12, {e6,1})

// ------------------------------ Mazmorra ------------------------------
SONG(SONG_TITLE, 110,
  {d5,2},{a5,2},{g5,1},{a5,1},{c6,2},{a5,4},{r,1},{f5,1},{g5,1},{a5,6})
SONG(SONG_TURN, 8, {c6,1})
SONG(SONG_BUMP, 30, {c4,1})
SONG(SONG_SWORD, 22, {a5,1},{d6,1})
SONG(SONG_HIT, 30, {e5,1},{c5,1})
SONG(SONG_KILL, 35, {c6,1},{g5,1},{e5,1},{c5,2})
SONG(SONG_HURT, 50, {f4,1},{c4,2})
SONG(SONG_RUPEE, 40, {e6,1},{b5,1},{e6,2})
SONG(SONG_HEART, 40, {a5,1},{e6,2})
SONG(SONG_CHEST, 70, {c5,1},{e5,1},{g5,1},{c6,1},{e6,4})
SONG(SONG_KEY, 60, {g5,1},{c6,1},{g5,1},{c6,3})
SONG(SONG_UNLOCK, 60, {c5,1},{g5,1},{c6,3})
SONG(SONG_NO, 60, {e4,1},{r,1},{e4,2})
SONG(SONG_CLEAR, 60, {g5,1},{c6,1},{e6,2})
SONG(SONG_STAIRS, 70, {c6,1},{a5,1},{f5,1},{d5,1},{a4,3})
SONG(SONG_BOMB, 25, {c6,1},{r,1},{c6,1})
SONG(SONG_BOOM, 22, {c4,1},{cs4,1},{c4,1},{d4,1},{c4,1},{cs4,1},{c4,3})
SONG(SONG_GAMEOVER, 140, {g4,3},{fs4,3},{f4,3},{e4,8})

// ------------------------------ General ------------------------------
SONG(SONG_MODE, 50, {g5,1},{b5,1},{d6,2})

// ------------------------------ Mensajes ------------------------------
// Llega el hada con una carta: campanitas que suben y bajan
SONG(SONG_LETTER, 55, {e6,1},{b5,1},{g6,1},{e6,1},{b5,1},{e6,3})
// Llamada entrante (se repite mientras suena)
SONG(SONG_CALL, 60, {a5,1},{e6,1},{a5,1},{e6,1},{r,1},{a5,1},{e6,1},{a5,1},{e6,1},{r,8})
SONG(SONG_PAGE, 20, {g5,1},{c6,1})
