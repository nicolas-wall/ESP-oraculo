// Mazmorra de bolsillo: roguelike por turnos, visto desde arriba.
//
// Izquierda / derecha: girar.  OK: avanzar (o atacar / abrir lo que tengas enfrente).
// Izquierda + derecha juntos: poner una bomba.  Mantener derecha: mapa del piso.
//
// Cada piso es una grilla de salas generada al azar. Hay que encontrar el cofre
// con la llave, abrir la escalera y bajar. Los enemigos se mueven solo cuando vos
// te movés.

#include "common.h"
#include "songs.h"

namespace {

// ----------------------------------------------------------------------------
// Sprites 8x8 (diseños propios)
// ----------------------------------------------------------------------------
const char* const S_WALL[] = {
  "########", "#...#...", "#...#...", "#...#...",
  "########", "..#...#.", "..#...#.", "..#...#.",
};
const char* const S_BLOCK[] = {
  "########", "#......#", "#.####.#", "#.#..#.#",
  "#.#..#.#", "#.####.#", "#......#", "########",
};
const char* const S_CHEST[] = {
  "........", ".######.", "#......#", "########",
  "#..##..#", "#......#", "########", "........",
};
const char* const S_CHEST_OPEN[] = {
  ".######.", "#.####.#", "........", "########",
  "#......#", "#......#", "########", "........",
};
const char* const S_LOCK[] = {
  "########", "#......#", "#..##..#", "#.#..#.#",
  "#.####.#", "#.####.#", "#......#", "########",
};
const char* const S_STAIRS[] = {
  "########", "#.......", "#.######", "#.#.....",
  "#.#.####", "#.#.#...", "#.#.#.##", "#.#.#.#.",
};
const char* const S_RUPEE[] = {
  "...#....", "..###...", ".#.#.#..", ".#.#.#..",
  ".#.#.#..", ".#.#.#..", "..###...", "...#....",
};
const char* const S_HEART[] = {
  "........", ".##.##..", "#######.", "#######.",
  ".#####..", "..###...", "...#....", "........",
};
const char* const S_BOMB[] = {
  "......#.", ".....#..", "..####..", ".######.",
  ".#.####.", ".######.", ".######.", "..####..",
};
const char* const S_KEY[] = {
  "........", ".##.....", "#..#....", "#..#####",
  "#..#.#.#", ".##.....", "........", "........",
};

// Héroe: N, E, S (O = E espejado)
const char* const S_HERO_N[] = {
  "..####..", ".######.", ".######.", ".######.",
  "..####..", ".#.##.#.", "..####..", "..#..#..",
};
const char* const S_HERO_E[] = {
  "..###...", ".#####..", ".###.#..", ".######.",
  "..###...", ".####.#.", "..###...", "..#.#...",
};
const char* const S_HERO_S[] = {
  "..####..", ".######.", ".#.##.#.", ".######.",
  "..####..", ".#.##.#.", "..####..", "..#..#..",
};

const char* const S_SLIME[2][8] = {
  {"........", "........", "..####..", ".######.",
   "##.##.##", "########", "########", ".######."},
  {"........", "...##...", "..####..", ".#.##.#.",
   ".######.", "########", "########", "########"},
};
const char* const S_BAT[2][8] = {
  {"#......#", "##....##", "###..###", ".#.##.#.",
   "..####..", "...##...", "........", "........"},
  {"........", "........", "...##...", "..####..",
   ".#.##.#.", "###..###", "##....##", "#......#"},
};
const char* const S_KNIGHT[2][8] = {
  {"..####..", ".#.##.#.", ".######.", "..#..#..",
   ".######.", "#.####.#", "..#..#..", ".##..##."},
  {"..####..", ".#.##.#.", ".######.", "..#..#..",
   ".######.", "#.####.#", "..#..#..", "..#..#.."},
};

// Íconos chicos del HUD
const char* const H_HEART[] = {".##.##.", "#######", "#######", ".#####.", "..###..", "...#..."};
const char* const H_HALF[] = {".##.##.", "####..#", "####..#", ".###.#.", "..###..", "...#..."};
const char* const H_EMPTY[] = {".##.##.", "#..#..#", "#.....#", ".#...#.", "..#.#..", "...#..."};
const char* const H_KEY[] = {".##.....", "#..#####", "#..#.#.#", ".##....."};
const char* const H_BOMB[] = {"....#", "...#.", ".###.", "#####", "#.###", "#####", ".###."};
const char* const H_RUPEE[] = {"..#..", ".###.", "#.#.#", "#.#.#", "#.#.#", ".###.", "..#.."};

// ----------------------------------------------------------------------------
// Mundo
// ----------------------------------------------------------------------------
const int RW = 16, RH = 7, TS = 8, OY = 8;  // sala de 16x7 casilleros, debajo del HUD
const int MAPW_MAX = 4, MAPH_MAX = 3;

enum Tile : uint8_t { T_FLOOR, T_WALL, T_BLOCK, T_CHEST, T_CHEST_OPEN, T_LOCK, T_STAIRS };
enum Dir : uint8_t { DN, DE, DS, DW };
const int DX[4] = {0, 1, 0, -1}, DY[4] = {-1, 0, 1, 0};
const uint8_t DOOR_BIT[4] = {1, 2, 4, 8};

// Obstáculos del interior (14x5). La fila del medio y las columnas 7-8 quedan
// siempre libres, así todas las puertas se conectan.
const char* const TEMPLATES[][5] = {
  {"..............", "..............", "..............", "..............", ".............."},
  {"..............", "..#........#..", "..............", "..#........#..", ".............."},
  {"##..........##", "#............#", "..............", "#............#", "##..........##"},
  {"...##....##...", "...#......#...", "..............", "...#......#...", "...##....##..."},
  {"..#........#..", "....#....#....", "..............", "....#....#....", "..#........#.."},
  {"..............", "....##..##....", "..............", "....##..##....", ".............."},
  {".#....##....#.", ".#..........#.", "..............", ".#..........#.", ".#....##....#."},
};
const int NUM_TMPL = sizeof(TEMPLATES) / sizeof(TEMPLATES[0]);

enum RoomKind : uint8_t { RK_NORMAL, RK_START, RK_STAIRS, RK_KEY, RK_TREASURE };
struct Room {
  uint8_t doors, tmpl, kind;
  bool visited, cleared, chestOpen, unlocked;
};

enum EType : uint8_t { E_SLIME, E_BAT, E_KNIGHT };
struct Enemy {
  int8_t x, y;
  uint8_t type, hp;
  bool alive;
  uint32_t hitAt;
};
const int MAX_EN = 6;

enum ItemT : uint8_t { I_NONE, I_RUPEE, I_HEART, I_BOMB };
struct Drop {
  int8_t x, y;
  uint8_t type;
};
const int MAX_DROPS = 6;

struct Bomb {
  int8_t x, y, fuse;
  bool active;
};

struct Fx {
  int8_t x, y;
  uint32_t at;
};

enum GState { G_TITLE, G_INTRO, G_PLAY, G_DEAD };

GState gst = G_TITLE;
uint32_t gstAt = 0;

int mapW = 3, mapH = 3;
Room rooms[MAPH_MAX][MAPW_MAX];
int curRX = 0, curRY = 0;
uint8_t tiles[RH][RW];
Enemy en[MAX_EN];
Drop drops[MAX_DROPS];
Bomb bomb;

int px = 7, py = 3, pdir = DS;
int hp = 6, maxHp = 6, bombs = 2, rupees = 0, floorN = 1;
bool hasKey = false;
uint32_t turnN = 0;

// Efectos
uint32_t hurtAt = 0, slashAt = 0, boomAt = 0;
int slashX = 0, slashY = 0, boomX = 0, boomY = 0;
Fx poofs[4];
int poofHead = 0;
uint32_t itemAt = 0;
int itemX = 0, itemY = 0;
const char* const* itemSprite = nullptr;
String toast;
uint32_t toastUntil = 0;
bool showMap = false;

int bestFloor = 0, bestRupees = 0;

// ----------------------------------------------------------------------------
bool inRoom(int x, int y) { return x >= 0 && x < RW && y >= 0 && y < RH; }
bool interior(int x, int y) { return x >= 1 && x < RW - 1 && y >= 1 && y < RH - 1; }
bool onBorder(int x, int y) { return x == 0 || y == 0 || x == RW - 1 || y == RH - 1; }

void say(const char* msg, uint32_t ms = 1300) {
  toast = msg;
  toastUntil = millis() + ms;
}

int enemyAt(int x, int y) {
  for (int i = 0; i < MAX_EN; i++)
    if (en[i].alive && en[i].x == x && en[i].y == y) return i;
  return -1;
}

int dropAt(int x, int y) {
  for (int i = 0; i < MAX_DROPS; i++)
    if (drops[i].type != I_NONE && drops[i].x == x && drops[i].y == y) return i;
  return -1;
}

bool bombAt(int x, int y) { return bomb.active && bomb.x == x && bomb.y == y; }

void addDrop(int x, int y, uint8_t type) {
  for (int i = 0; i < MAX_DROPS; i++)
    if (drops[i].type == I_NONE) {
      drops[i] = {(int8_t)x, (int8_t)y, type};
      return;
    }
}

void addPoof(int x, int y) {
  poofs[poofHead] = {(int8_t)x, (int8_t)y, millis()};
  poofHead = (poofHead + 1) % 4;
}

Room& curRoom() { return rooms[curRY][curRX]; }

// Lugar fijo (para la misma sala siempre el mismo) donde va un cofre o la escalera.
bool pickSpot(int salt, int& ox, int& oy) {
  int cx[70], cy[70], n = 0;
  for (int y = 1; y < RH - 1; y++)
    for (int x = 2; x < RW - 2; x++) {
      if (y == 3 || x == 7 || x == 8) continue;
      if (tiles[y][x] != T_FLOOR) continue;
      cx[n] = x;
      cy[n] = y;
      n++;
    }
  if (!n) return false;
  int i = salt % n;
  ox = cx[i];
  oy = cy[i];
  return true;
}

void spawnEnemies() {
  Room& r = curRoom();
  int n = 1 + floorN / 2 + random(2);
  if (r.kind == RK_STAIRS) n++;
  if (n > MAX_EN) n = MAX_EN;
  int knightPct = floorN < 2 ? 0 : min(20 + floorN * 6, 55);
  for (int i = 0; i < n; i++) {
    for (int tries = 0; tries < 40; tries++) {
      int x = 1 + random(RW - 2), y = 1 + random(RH - 2);
      if (tiles[y][x] != T_FLOOR || enemyAt(x, y) >= 0) continue;
      if (abs(x - px) + abs(y - py) < 4) continue;
      Enemy& e = en[i];
      e.type = (int)random(100) < knightPct ? E_KNIGHT : (random(2) ? E_SLIME : E_BAT);
      e.hp = e.type == E_KNIGHT ? 2 : 1;
      e.x = x;
      e.y = y;
      e.alive = true;
      e.hitAt = 0;
      break;
    }
  }
}

void loadRoom(int rx, int ry, int ex, int ey) {
  curRX = rx;
  curRY = ry;
  Room& r = curRoom();
  for (int y = 0; y < RH; y++)
    for (int x = 0; x < RW; x++) tiles[y][x] = onBorder(x, y) ? T_WALL : T_FLOOR;
  if (r.doors & 1) tiles[0][7] = tiles[0][8] = T_FLOOR;
  if (r.doors & 4) tiles[RH - 1][7] = tiles[RH - 1][8] = T_FLOOR;
  if (r.doors & 8) tiles[3][0] = T_FLOOR;
  if (r.doors & 2) tiles[3][RW - 1] = T_FLOOR;
  for (int y = 0; y < 5; y++)
    for (int x = 0; x < 14; x++)
      if (TEMPLATES[r.tmpl][y][x] == '#') tiles[y + 1][x + 1] = T_BLOCK;

  int sx, sy, salt = rx * 31 + ry * 17 + floorN * 7;
  if ((r.kind == RK_KEY || r.kind == RK_TREASURE) && pickSpot(salt, sx, sy))
    tiles[sy][sx] = r.chestOpen ? T_CHEST_OPEN : T_CHEST;
  if (r.kind == RK_STAIRS && pickSpot(salt, sx, sy)) tiles[sy][sx] = r.unlocked ? T_STAIRS : T_LOCK;

  for (int i = 0; i < MAX_DROPS; i++) drops[i].type = I_NONE;
  for (int i = 0; i < MAX_EN; i++) en[i].alive = false;
  bomb.active = false;
  px = ex;
  py = ey;
  if (!r.cleared && r.kind != RK_START) spawnEnemies();
  r.visited = true;
}

void genFloor() {
  mapW = floorN >= 3 ? 4 : 3;
  mapH = 3;
  memset(rooms, 0, sizeof(rooms));

  // Laberinto de salas (recorrido en profundidad al azar)
  bool seen[MAPH_MAX][MAPW_MAX] = {};
  int stX[12], stY[12], sp = 0;
  int sx = random(mapW), sy = random(mapH);
  stX[sp] = sx;
  stY[sp] = sy;
  sp++;
  seen[sy][sx] = true;
  while (sp > 0) {
    int cx = stX[sp - 1], cy = stY[sp - 1];
    int opts[4], n = 0;
    for (int d = 0; d < 4; d++) {
      int nx = cx + DX[d], ny = cy + DY[d];
      if (nx >= 0 && nx < mapW && ny >= 0 && ny < mapH && !seen[ny][nx]) opts[n++] = d;
    }
    if (!n) {
      sp--;
      continue;
    }
    int d = opts[random(n)];
    int nx = cx + DX[d], ny = cy + DY[d];
    rooms[cy][cx].doors |= DOOR_BIT[d];
    rooms[ny][nx].doors |= DOOR_BIT[(d + 2) % 4];
    seen[ny][nx] = true;
    stX[sp] = nx;
    stY[sp] = ny;
    sp++;
  }
  // Algunas puertas extra para que haya vueltas
  int extra = 1 + floorN / 3;
  for (int k = 0; k < 20 && extra > 0; k++) {
    int cx = random(mapW), cy = random(mapH), d = random(4);
    int nx = cx + DX[d], ny = cy + DY[d];
    if (nx < 0 || nx >= mapW || ny < 0 || ny >= mapH) continue;
    if (rooms[cy][cx].doors & DOOR_BIT[d]) continue;
    rooms[cy][cx].doors |= DOOR_BIT[d];
    rooms[ny][nx].doors |= DOOR_BIT[(d + 2) % 4];
    extra--;
  }

  // Distancias desde el inicio: la escalera va en la sala más lejana
  int dist[MAPH_MAX][MAPW_MAX];
  for (int y = 0; y < MAPH_MAX; y++)
    for (int x = 0; x < MAPW_MAX; x++) dist[y][x] = -1;
  int qx[12], qy[12], qh = 0, qt = 0;
  dist[sy][sx] = 0;
  qx[qt] = sx;
  qy[qt] = sy;
  qt++;
  while (qh < qt) {
    int cx = qx[qh], cy = qy[qh];
    qh++;
    for (int d = 0; d < 4; d++) {
      if (!(rooms[cy][cx].doors & DOOR_BIT[d])) continue;
      int nx = cx + DX[d], ny = cy + DY[d];
      if (dist[ny][nx] >= 0) continue;
      dist[ny][nx] = dist[cy][cx] + 1;
      qx[qt] = nx;
      qy[qt] = ny;
      qt++;
    }
  }
  int fx = sx, fy = sy;
  for (int y = 0; y < mapH; y++)
    for (int x = 0; x < mapW; x++)
      if (dist[y][x] > dist[fy][fx]) {
        fx = x;
        fy = y;
      }
  // La llave, en la sala más lejana que quede (empates al azar)
  int kx = -1, ky = -1, best = -1;
  for (int y = 0; y < mapH; y++)
    for (int x = 0; x < mapW; x++) {
      if ((x == sx && y == sy) || (x == fx && y == fy)) continue;
      int score = dist[y][x] * 10 + random(10);
      if (score > best) {
        best = score;
        kx = x;
        ky = y;
      }
    }
  // Un cofre con premio en otra sala al azar
  int tx = -1, ty = -1;
  for (int tries = 0; tries < 30; tries++) {
    int x = random(mapW), y = random(mapH);
    if ((x == sx && y == sy) || (x == fx && y == fy) || (x == kx && y == ky)) continue;
    tx = x;
    ty = y;
    break;
  }

  for (int y = 0; y < mapH; y++)
    for (int x = 0; x < mapW; x++) rooms[y][x].tmpl = random(NUM_TMPL);
  rooms[sy][sx].kind = RK_START;
  rooms[sy][sx].tmpl = 0;
  rooms[sy][sx].cleared = true;
  rooms[fy][fx].kind = RK_STAIRS;
  if (kx >= 0) rooms[ky][kx].kind = RK_KEY;
  if (tx >= 0) rooms[ty][tx].kind = RK_TREASURE;

  hasKey = false;
  pdir = DS;
  loadRoom(sx, sy, 7, 3);
}

// ----------------------------------------------------------------------------
// Turnos
// ----------------------------------------------------------------------------
void saveRecord() {
  if (floorN > bestFloor || (floorN == bestFloor && rupees > bestRupees)) {
    bestFloor = floorN;
    bestRupees = rupees;
    prefs.putInt("bestFloor", bestFloor);
    prefs.putInt("bestRup", bestRupees);
  }
}

void hurtPlayer(int dmg) {
  hp -= dmg;
  hurtAt = millis();
  if (hp <= 0) {
    hp = 0;
    saveRecord();
    gst = G_DEAD;
    gstAt = millis();
    music.play(&SONG_GAMEOVER);
  } else {
    music.play(&SONG_HURT);
  }
}

void checkCleared() {
  for (int i = 0; i < MAX_EN; i++)
    if (en[i].alive) return;
  Room& r = curRoom();
  if (!r.cleared) {
    r.cleared = true;
    music.play(&SONG_CLEAR);
  }
}

void damageEnemy(int i, int dmg) {
  Enemy& e = en[i];
  e.hitAt = millis();
  if (e.hp > dmg) {
    e.hp -= dmg;
    music.play(&SONG_HIT);
    return;
  }
  e.alive = false;
  addPoof(e.x, e.y);
  music.play(&SONG_KILL);
  int roll = random(100);
  if (roll < 35) addDrop(e.x, e.y, I_RUPEE);
  else if (roll < 50 && hp < maxHp) addDrop(e.x, e.y, I_HEART);
  else if (roll < 56) addDrop(e.x, e.y, I_BOMB);
  checkCleared();
}

bool enemyCanEnter(int x, int y) {
  return interior(x, y) && tiles[y][x] == T_FLOOR && enemyAt(x, y) < 0 && !bombAt(x, y);
}

void explode() {
  bomb.active = false;
  boomAt = millis();
  boomX = bomb.x;
  boomY = bomb.y;
  music.play(&SONG_BOOM);
  for (int dy = -1; dy <= 1; dy++)
    for (int dx = -1; dx <= 1; dx++) {
      int x = bomb.x + dx, y = bomb.y + dy;
      if (!interior(x, y)) continue;
      if (tiles[y][x] == T_BLOCK) tiles[y][x] = T_FLOOR;  // las bombas rompen bloques
      int e = enemyAt(x, y);
      if (e >= 0) damageEnemy(e, 2);
    }
  if (abs(px - bomb.x) <= 1 && abs(py - bomb.y) <= 1) hurtPlayer(2);
}

void enemiesAct() {
  for (int i = 0; i < MAX_EN && gst == G_PLAY; i++) {
    Enemy& e = en[i];
    if (!e.alive) continue;
    if (e.type == E_SLIME && (turnN + i) % 2) continue;  // los blobs son lentos

    int dx = px - e.x, dy = py - e.y;
    int toward = abs(dx) >= abs(dy) ? (dx > 0 ? DE : DW) : (dy > 0 ? DS : DN);
    int side = abs(dx) >= abs(dy) ? (dy > 0 ? DS : DN) : (dx > 0 ? DE : DW);
    if ((abs(dx) >= abs(dy) ? dy : dx) == 0) side = -1;

    int tries[2] = {-1, -1};
    switch (e.type) {
      case E_SLIME:
        if (random(4) == 0) continue;
        tries[0] = random(4);
        break;
      case E_BAT:
        tries[0] = random(2) ? toward : random(4);
        break;
      case E_KNIGHT:
        tries[0] = toward;
        tries[1] = side;
        break;
    }
    for (int k = 0; k < 2; k++) {
      int d = tries[k];
      if (d < 0) continue;
      int nx = e.x + DX[d], ny = e.y + DY[d];
      if (nx == px && ny == py) {
        e.hitAt = millis();
        hurtPlayer(e.type == E_KNIGHT ? 2 : 1);
        break;
      }
      if (enemyCanEnter(nx, ny)) {
        e.x = nx;
        e.y = ny;
        break;
      }
    }
  }
}

void endTurn() {
  turnN++;
  if (bomb.active && --bomb.fuse <= 0) explode();
  if (gst == G_PLAY) enemiesAct();
}

void pickUp() {
  int d = dropAt(px, py);
  if (d < 0) return;
  switch (drops[d].type) {
    case I_RUPEE: rupees++; music.play(&SONG_RUPEE); break;
    case I_HEART: hp = min(hp + 2, maxHp); music.play(&SONG_HEART); break;
    case I_BOMB: bombs++; music.play(&SONG_RUPEE); break;
  }
  drops[d].type = I_NONE;
}

void showItem(int x, int y, const char* const* spr) {
  itemAt = millis();
  itemX = x;
  itemY = y;
  itemSprite = spr;
}

void openChest(int x, int y) {
  Room& r = curRoom();
  tiles[y][x] = T_CHEST_OPEN;
  r.chestOpen = true;
  if (r.kind == RK_KEY) {
    hasKey = true;
    showItem(x, y, S_KEY);
    say("¡Una llave!");
    music.play(&SONG_CHEST, &SONG_KEY);
  } else if (floorN % 2 == 0 && maxHp < 12) {
    maxHp += 2;
    hp = maxHp;
    showItem(x, y, S_HEART);
    say("¡Corazón extra!");
    music.play(&SONG_CHEST, &SONG_HEART);
  } else if (random(2)) {
    bombs += 3;
    showItem(x, y, S_BOMB);
    say("¡3 bombas!");
    music.play(&SONG_CHEST);
  } else {
    rupees += 5;
    showItem(x, y, S_RUPEE);
    say("¡5 rupias!");
    music.play(&SONG_CHEST, &SONG_RUPEE);
  }
}

void changeRoom() {
  int nrx = curRX + DX[pdir], nry = curRY + DY[pdir];
  if (nrx < 0 || nrx >= mapW || nry < 0 || nry >= mapH) return;
  bool anyAlive = false;
  for (int i = 0; i < MAX_EN; i++) anyAlive |= en[i].alive;
  if (!anyAlive) curRoom().cleared = true;
  int ex = px, ey = py;
  switch (pdir) {
    case DN: ey = RH - 2; break;
    case DS: ey = 1; break;
    case DE: ex = 1; ey = 3; break;
    case DW: ex = RW - 2; ey = 3; break;
  }
  loadRoom(nrx, nry, ex, ey);
}

void stepForward() {
  int tx = px + DX[pdir], ty = py + DY[pdir];
  if (!inRoom(tx, ty)) return;

  int e = enemyAt(tx, ty);
  if (e >= 0) {
    slashAt = millis();
    slashX = tx;
    slashY = ty;
    music.play(&SONG_SWORD);
    damageEnemy(e, 1);
    endTurn();
    return;
  }
  if (bombAt(tx, ty)) {
    music.play(&SONG_BUMP);
    return;
  }
  switch (tiles[ty][tx]) {
    case T_CHEST:
      openChest(tx, ty);
      endTurn();
      return;
    case T_LOCK:
      if (hasKey) {
        hasKey = false;
        tiles[ty][tx] = T_STAIRS;
        curRoom().unlocked = true;
        say("¡Se abrió!");
        music.play(&SONG_UNLOCK);
        endTurn();
      } else {
        say("Falta la llave");
        music.play(&SONG_NO);
      }
      return;
    case T_WALL:
    case T_BLOCK:
    case T_CHEST_OPEN:
      music.play(&SONG_BUMP);
      return;
    default:
      break;
  }

  px = tx;
  py = ty;
  pickUp();
  if (tiles[py][px] == T_STAIRS) {
    floorN++;
    saveRecord();
    gst = G_INTRO;
    gstAt = millis();
    music.play(&SONG_STAIRS);
    return;
  }
  if (onBorder(px, py)) {
    changeRoom();
    return;
  }
  endTurn();
}

void placeBomb() {
  int tx = px + DX[pdir], ty = py + DY[pdir];
  if (bombs <= 0) {
    say("Sin bombas");
    music.play(&SONG_NO);
    return;
  }
  if (bomb.active) {
    say("Ya hay una bomba");
    music.play(&SONG_NO);
    return;
  }
  if (!interior(tx, ty) || tiles[ty][tx] != T_FLOOR || enemyAt(tx, ty) >= 0) {
    music.play(&SONG_BUMP);
    return;
  }
  bombs--;
  bomb = {(int8_t)tx, (int8_t)ty, 4, true};
  music.play(&SONG_BOMB);
  endTurn();
}

void newGame() {
  floorN = 1;
  maxHp = 6;
  hp = 6;
  bombs = 2;
  rupees = 0;
  turnN = 0;
  showMap = false;
  gst = G_INTRO;
  gstAt = millis();
  music.play(&SONG_TITLE);
}

// ----------------------------------------------------------------------------
// Dibujo
// ----------------------------------------------------------------------------
void drawTile(int x, int y) {
  int sx = x * TS, sy = OY + y * TS;
  switch (tiles[y][x]) {
    case T_WALL: drawSprite(sx, sy, S_WALL, 8); break;
    case T_BLOCK: drawSprite(sx, sy, S_BLOCK, 8); break;
    case T_CHEST: drawSprite(sx, sy, S_CHEST, 8); break;
    case T_CHEST_OPEN: drawSprite(sx, sy, S_CHEST_OPEN, 8); break;
    case T_LOCK: drawSprite(sx, sy, S_LOCK, 8); break;
    case T_STAIRS: drawSprite(sx, sy, S_STAIRS, 8); break;
    case T_FLOOR:
      if (!onBorder(x, y) && (x + y) % 2 == 0) u8g2.drawPixel(sx + 4, sy + 4);
      break;
  }
}

void drawHud() {
  for (int i = 0; i < maxHp / 2; i++) {
    int v = hp - i * 2;
    drawSprite(i * 8, 1, v >= 2 ? H_HEART : (v == 1 ? H_HALF : H_EMPTY), 6);
  }
  if (hasKey) drawSprite(50, 2, H_KEY, 4);
  u8g2.setFont(u8g2_font_5x7_tf);
  char buf[8];
  drawSprite(62, 0, H_BOMB, 7);
  snprintf(buf, sizeof(buf), "%d", bombs);
  u8g2.drawStr(68, 7, buf);
  drawSprite(82, 0, H_RUPEE, 7);
  snprintf(buf, sizeof(buf), "%d", rupees);
  u8g2.drawStr(88, 7, buf);
  snprintf(buf, sizeof(buf), "P%d", floorN);
  u8g2.drawStr(128 - u8g2.getStrWidth(buf), 7, buf);
}

void drawEnemy(const Enemy& e, uint32_t now) {
  int sx = e.x * TS, sy = OY + e.y * TS, f = (now / 300 + e.x) % 2;
  const char* const* spr = e.type == E_SLIME ? S_SLIME[f] : (e.type == E_BAT ? S_BAT[f] : S_KNIGHT[f]);
  drawSprite(sx, sy, spr, 8);
  if (now - e.hitAt < 150) {  // destello al recibir o dar un golpe
    u8g2.setDrawColor(2);
    u8g2.drawBox(sx, sy, 8, 8);
    u8g2.setDrawColor(1);
  }
}

void drawPlayer(uint32_t now) {
  if (now - hurtAt < 400 && (now / 60) % 2) return;  // parpadea al recibir daño
  int sx = px * TS, sy = OY + py * TS;
  switch (pdir) {
    case DN: drawSprite(sx, sy, S_HERO_N, 8); break;
    case DE: drawSprite(sx, sy, S_HERO_E, 8); break;
    case DS: drawSprite(sx, sy, S_HERO_S, 8); break;
    case DW: drawSpriteMirror(sx, sy, S_HERO_E, 8); break;
  }
  // Marca el casillero de enfrente (adonde vas a avanzar)
  int tx = px + DX[pdir], ty = py + DY[pdir];
  if (inRoom(tx, ty) && (now / 250) % 3) {
    int ax = tx * TS, ay = OY + ty * TS;
    u8g2.drawPixel(ax, ay);
    u8g2.drawPixel(ax + 7, ay);
    u8g2.drawPixel(ax, ay + 7);
    u8g2.drawPixel(ax + 7, ay + 7);
  }
}

void drawEffects(uint32_t now) {
  if (now - slashAt < 140) {
    int cx = slashX * TS + 4, cy = OY + slashY * TS + 4;
    if (pdir == DN || pdir == DS) u8g2.drawVLine(cx, cy - 3, 7);
    else u8g2.drawHLine(cx - 3, cy, 7);
  }
  for (int i = 0; i < 4; i++) {
    uint32_t t = now - poofs[i].at;
    if (poofs[i].at && t < 300) {
      int cx = poofs[i].x * TS + 4, cy = OY + poofs[i].y * TS + 4, d = 1 + t / 60;
      drawSparkle(cx - d, cy - d, 1);
      drawSparkle(cx + d, cy - d, 1);
      drawSparkle(cx - d, cy + d, 1);
      drawSparkle(cx + d, cy + d, 1);
    }
  }
  if (now - boomAt < 350) {
    int cx = boomX * TS + 4, cy = OY + boomY * TS + 4, r = 3 + (now - boomAt) / 30;
    u8g2.drawCircle(cx, cy, r);
    if (r > 6) u8g2.drawCircle(cx, cy, r - 5);
  }
  if (itemSprite && now - itemAt < 1200) {
    int rise = min((int)(now - itemAt) / 60, 8);
    drawSprite(itemX * TS, OY + itemY * TS - rise, itemSprite, 8);
  }
}

void drawMap(uint32_t now) {
  u8g2.setDrawColor(0);
  u8g2.drawBox(0, 0, 128, 64);
  u8g2.setDrawColor(1);
  u8g2.setFont(u8g2_font_5x7_tf);
  char buf[16];
  snprintf(buf, sizeof(buf), "MAPA - PISO %d", floorN);
  centerText(7, buf);
  const int cw = 20, chh = 12, gap = 6;
  int ox = (128 - (mapW * cw + (mapW - 1) * gap)) / 2, oy = 12;
  for (int y = 0; y < mapH; y++)
    for (int x = 0; x < mapW; x++) {
      Room& r = rooms[y][x];
      int rx = ox + x * (cw + gap), ry = oy + y * (chh + gap);
      if (!r.visited) {
        u8g2.drawPixel(rx + cw / 2, ry + chh / 2);
        continue;
      }
      u8g2.drawFrame(rx, ry, cw, chh);
      if (r.doors & 2) u8g2.drawHLine(rx + cw, ry + chh / 2, gap);
      if (r.doors & 4) u8g2.drawVLine(rx + cw / 2, ry + chh, gap);
      if (r.kind == RK_STAIRS) u8g2.drawStr(rx + 8, ry + 9, r.unlocked ? ">" : "#");
      if ((r.kind == RK_KEY || r.kind == RK_TREASURE) && !r.chestOpen) u8g2.drawStr(rx + 8, ry + 9, "?");
      if (x == curRX && y == curRY && (now / 300) % 2) u8g2.drawBox(rx + 2, ry + 2, cw - 4, chh - 4);
    }
}

void drawTitle(uint32_t now) {
  u8g2.drawFrame(0, 0, 128, 64);
  u8g2.setFont(u8g2_font_ncenB14_tf);
  centerText(22, "MAZMORRA");
  // héroe y un blob que se miran
  drawSprite(46, 30, S_HERO_E, 8);
  drawSprite(74, 30, S_SLIME[(now / 300) % 2], 8);
  if ((now / 200) % 2) u8g2.drawHLine(56, 34, 4);
  u8g2.setFont(u8g2_font_5x7_tf);
  if ((now / 500) % 2) centerText(50, "OK PARA ENTRAR");
  if (bestFloor > 0) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Récord: piso %d, %d rupias", bestFloor, bestRupees);
    centerText(60, buf);
  }
}

void drawIntro(uint32_t now) {
  uint32_t t = now - gstAt;
  u8g2.setFont(u8g2_font_ncenB14_tf);
  char buf[16];
  snprintf(buf, sizeof(buf), "PISO %d", floorN);
  centerText(30, buf);
  // escalones que bajan
  int steps = min((int)(t / 120), 6);
  for (int i = 0; i < steps; i++) u8g2.drawBox(40 + i * 8, 40 + i * 3, 8, 2);
  u8g2.setFont(u8g2_font_5x7_tf);
  if (floorN == 1) centerText(62, "Buscá la llave y bajá");
}

void drawDead(uint32_t now) {
  u8g2.setFont(u8g2_font_ncenB10_tf);
  centerText(16, "CAÍSTE");
  u8g2.setFont(u8g2_font_6x10_tf);
  char buf[32];
  snprintf(buf, sizeof(buf), "Piso %d  -  %d rupias", floorN, rupees);
  centerText(32, buf);
  snprintf(buf, sizeof(buf), "Récord: piso %d", bestFloor);
  centerText(44, buf);
  if (now - gstAt > 1500 && (now / 500) % 2) {
    u8g2.setFont(u8g2_font_5x7_tf);
    centerText(60, "OK PARA VOLVER A ENTRAR");
  }
}

}  // namespace

// ----------------------------------------------------------------------------
void dungeonBegin() {
  bestFloor = prefs.getInt("bestFloor", 0);
  bestRupees = prefs.getInt("bestRup", 0);
}

void dungeonEnter(uint32_t now) {
  showMap = false;
  if (gst == G_DEAD) gst = G_TITLE;
  if (gst == G_TITLE) gstAt = now;
}

void dungeonUpdate(uint32_t now) {
  switch (gst) {
    case G_TITLE:
      if (btnOk.shortRel) newGame();
      break;

    case G_INTRO:
      if (now - gstAt > 1600 || (btnOk.shortRel && now - gstAt > 400)) {
        genFloor();
        gst = G_PLAY;
        gstAt = now;
      }
      break;

    case G_PLAY:
      if (showMap) {
        if (btnL.shortRel || btnOk.shortRel || btnR.shortRel || btnR.longPress) showMap = false;
        break;
      }
      if (btnR.longPress) {
        showMap = true;
        break;
      }
      if (comboFired) placeBomb();
      else if (btnL.shortRel) {
        pdir = (pdir + 3) % 4;
        music.play(&SONG_TURN);
      } else if (btnR.shortRel) {
        pdir = (pdir + 1) % 4;
        music.play(&SONG_TURN);
      } else if (btnOk.shortRel) {
        stepForward();
      }
      break;

    case G_DEAD:
      if (btnOk.shortRel && now - gstAt > 1500) {
        gst = G_TITLE;
        gstAt = now;
      }
      break;
  }
}

void dungeonDraw(uint32_t now) {
  switch (gst) {
    case G_TITLE: drawTitle(now); return;
    case G_INTRO: drawIntro(now); return;
    case G_DEAD: drawDead(now); return;
    case G_PLAY: break;
  }
  if (showMap) {
    drawMap(now);
    return;
  }
  drawHud();
  for (int y = 0; y < RH; y++)
    for (int x = 0; x < RW; x++) drawTile(x, y);
  for (int i = 0; i < MAX_DROPS; i++) {
    const Drop& d = drops[i];
    if (d.type == I_NONE) continue;
    const char* const* spr = d.type == I_RUPEE ? S_RUPEE : (d.type == I_HEART ? S_HEART : S_BOMB);
    drawSprite(d.x * TS, OY + d.y * TS, spr, 8);
  }
  if (bomb.active && !(bomb.fuse <= 1 && (now / 100) % 2)) drawSprite(bomb.x * TS, OY + bomb.y * TS, S_BOMB, 8);
  for (int i = 0; i < MAX_EN; i++)
    if (en[i].alive) drawEnemy(en[i], now);
  drawPlayer(now);
  drawEffects(now);
  if ((int32_t)(toastUntil - now) > 0) drawToast(toast.c_str());
}
