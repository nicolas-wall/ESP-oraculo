#pragma once

// Sprites dibujados como texto: '#' = pixel encendido. Todos son diseños propios.

// Sabio de la cueva: capucha, barba larga y túnica.
static const char* const SPR_SAGE[] = {
  ".....###.....",
  "....#####....",
  "...##...##...",
  "...#.#.#.#...",
  "...#.....#...",
  "...##.#.##...",
  "..##.###.##..",
  ".#.#.###.#.#.",
  "#..#.###.#..#",
  "#..#..#..#..#",
  "...#.....#...",
  "..#.......#..",
  "..#.......#..",
  ".#.........#.",
  ".#.........#.",
  ".###########.",
};

// Fuego (se dibuja espejado en frames alternos para que titile)
static const char* const SPR_FLAME[] = {
  "....#....",
  "...##....",
  "...###...",
  "..####...",
  "..#####..",
  ".###.##..",
  ".##..###.",
  ".##...##.",
  ".###.###.",
  "..#####..",
  "...###...",
};

// Hada mensajera: alas grandes, cuerpo chiquito (dos cuadros para aletear).
static const char* const SPR_FAIRY_A[] = {
  "##.....##",
  "###...###",
  "####.####",
  ".###.###.",
  "..#####..",
  "...###...",
  "..#.#.#..",
  "....#....",
};
static const char* const SPR_FAIRY_B[] = {
  ".........",
  ".........",
  "....#....",
  "##.###.##",
  "#########",
  ".#######.",
  "..#.#.#..",
  "....#....",
};

// Carta cerrada
static const char* const SPR_LETTER[] = {
  "#############",
  "##.........##",
  "#.#.......#.#",
  "#..#.....#..#",
  "#...#...#...#",
  "#....#.#....#",
  "#.....#.....#",
  "#...........#",
  "#############",
};
