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
  c6, cs6, d6, ds6, e6, f6, fs6, g6, gs6, a6, as6, b6,
  c7
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
// La respuesta aparece: tema místico del sabio
SONG(SONG_REVEAL, 75,
  {d5,1},{a5,1},{d6,1},{f6,2},{e6,1},{c6,1},{d6,2},{a5,1},{d6,4})
SONG(SONG_BLIP, 12, {e6,1})

// ------------------------------ Mazmorra ------------------------------
// Motivo de aventura del título
SONG(SONG_TITLE, 105,
  {d5,2},{a5,2},{d6,3},{c6,1},{a5,2},{f5,2},{g5,1},{a5,5})
SONG(SONG_TURN, 8, {c6,1})
SONG(SONG_BUMP, 25, {d4,1},{c4,2})
// Espadazo: barrido rápido hacia arriba
SONG(SONG_SWORD, 12, {e5,1},{a5,1},{d6,1},{g6,1})
// Golpe seco
SONG(SONG_HIT, 25, {g4,1},{c4,2})
// "Puf" que baja y termina en un brillito
SONG(SONG_KILL, 28, {g6,1},{d6,1},{a5,1},{e5,1},{r,1},{e6,1},{b6,2})
// Daño: dos graves desafinadas
SONG(SONG_HURT, 45, {cs4,2},{c4,3})
// Rupia: "tiling"
SONG(SONG_RUPEE, 28, {d6,1},{a6,1},{d6,1},{a6,2})
SONG(SONG_HEART, 40, {a5,1},{cs6,1},{e6,1},{a6,2})
// Fanfarria de cofre: "¡conseguiste algo!"
SONG(SONG_CHEST, 75, {c5,1},{e5,1},{a5,1},{c6,2},{b5,1},{c6,1},{e6,4})
SONG(SONG_KEY, 60, {a5,1},{e6,1},{a6,3},{g6,1},{a6,2})
// Secreto: arpegio misterioso que sube
SONG(SONG_UNLOCK, 60, {d5,1},{f5,1},{a5,1},{cs6,1},{e6,1},{a6,4})
SONG(SONG_NO, 60, {e4,1},{r,1},{e4,2})
SONG(SONG_CLEAR, 60, {e5,1},{a5,1},{cs6,1},{e6,3})
SONG(SONG_STAIRS, 55, {a6,1},{e6,1},{cs6,1},{a5,1},{e5,1},{a4,3})
SONG(SONG_BOMB, 25, {c6,1},{r,1},{c6,1})
SONG(SONG_BOOM, 20, {d4,1},{c4,1},{ds4,1},{c4,1},{cs4,1},{c4,4})
SONG(SONG_GAMEOVER, 130, {a4,3},{gs4,3},{g4,3},{fs4,6})

// ------------------------------ General ------------------------------
SONG(SONG_MODE, 50, {e5,1},{a5,1},{e6,2})

// ------------------------------ Cartas ------------------------------
// Llamado del hada: "¡eh, mirá!"
SONG(SONG_LETTER, 45, {g6,1},{d6,1},{a6,2},{r,2},{g6,1},{d6,1},{a6,3})
// La carta se abre: arpa que sube
SONG(SONG_LETTER_OPEN, 40, {d5,1},{fs5,1},{a5,1},{d6,1},{fs6,1},{a6,3})
// Se cierra: brillito que baja
SONG(SONG_LETTER_CLOSE, 35, {a6,1},{e6,1},{a5,2})
// Llamada entrante: melodía de ocarina inventada (se repite mientras suena)
SONG(SONG_CALL, 120,
  {d5,2},{a5,2},{g5,1},{f5,1},{e5,2},{r,1},{c5,1},{e5,1},{g5,3},{r,6})
SONG(SONG_PAGE, 20, {g5,1},{c6,1})
