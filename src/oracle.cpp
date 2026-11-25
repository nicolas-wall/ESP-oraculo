// Oráculo: una esfera entre dos fuegos. Lo agitás (o apretás un botón),
// la esfera tiembla y el sabio aparece con un consejo.

#include "common.h"
#include "songs.h"
#include "sprites.h"

namespace {

const char* const PHRASES[] = {
  "Las estrellas dicen que sí.",
  "Ni lo sueñes, viajero.",
  "Preguntá de nuevo cuando salga la luna.",
  "El cofre que buscás está más cerca de lo que creés.",
  "Sí, pero llevá corazones de repuesto.",
  "No confíes en ninguna gallina hoy.",
  "Las rupias van y vienen. La siesta queda.",
  "El destino dice que sí. El hada no está tan segura.",
  "Hoy es buen día para romper vasijas.",
  "Mejor no. Hay un murciélago esperando.",
  "Antes de decidir, tomá agua.",
  "El viento dice que no, y el viento nunca miente.",
  "Probá por el camino que todavía no exploraste.",
  "Si tenés que preguntar dos veces, ya sabés la respuesta.",
  "Una llave pequeña abre una gran puerta.",
  "Esa puerta está cerrada por un motivo.",
  "Seguí al hada. Ella sabe.",
  "Sí, después de una buena siesta.",
  "Ni las piedras lo saben todavía.",
  "Dudoso. Muy dudoso.",
  "Totalmente. Ya está escrito en la piedra.",
  "La respuesta está debajo de una roca.",
  "Hacelo. Como mucho perdés medio corazón.",
  "No. Y no insistas, que despertás a los fuegos.",
  "Los atajos siempre tienen murciélagos.",
  "Hoy no. Mañana, tal vez.",
  "Guardá esa pregunta para la luna llena.",
  "Un héroe sabio come antes de pelear.",
  "Tu suerte está en el cofre que no abriste.",
  "Sí. Pero avisale a alguien a dónde vas.",
  "El sabio dice que sí. El sabio a veces se equivoca.",
  "Las vasijas rotas no se lamentan. Seguí.",
  "Sí, y con música de fondo.",
  "Mirá bien las paredes agrietadas.",
  "Respuesta nublada. Agitá otra vez.",
  "Pregunta difícil. Necesito más rupias.",
  "No ahora. El fuego dice que esperes.",
  "Sin ninguna duda.",
  "Mis visiones dicen que no.",
  "Contá hasta tres y elegí lo primero que pienses.",
  "La paciencia abre más puertas que la espada.",
  "No todo lo que brilla es una rupia.",
  "Lo que buscás ya está en tu inventario.",
  "Sí. El oráculo lo vio en sueños.",
  "Las runas dicen que ni ahí.",
  "Primero ordená tu inventario.",
  "Claro que sí, héroe.",
  "El eco de la cueva responde: quizás.",
};
const int NPH = sizeof(PHRASES) / sizeof(PHRASES[0]);

enum OState { O_IDLE, O_SHAKE, O_REVEAL, O_DONE, O_SLEEP };
OState ost = O_IDLE;
uint32_t ostAt = 0, lastAct = 0;
TypeBox box;
int lastTyped = 0, lastPhrase = -1;

const uint32_t SHAKE_MS = 1500;      // cuánto "piensa"
const uint32_t TYPE_DELAY = 900;     // espera antes de escribir (deja sonar el acorde)
const uint32_t DONE_TIMEOUT = 30000; // después vuelve a la esfera

void ask(uint32_t now) {
  ost = O_SHAKE;
  ostAt = now;
  music.play(&SONG_RUMBLE);
}

void drawFlames(int lx, int rx, int y, uint32_t now, bool big) {
  bool f = (now / (big ? 70 : 140)) % 2;
  for (int k = 0; k < (big ? 2 : 1); k++) {
    int yy = y - k * 5;
    if (f ^ k) {
      drawSprite(lx, yy, SPR_FLAME, 11);
      drawSpriteMirror(rx, yy, SPR_FLAME, 11);
    } else {
      drawSpriteMirror(lx, yy, SPR_FLAME, 11);
      drawSprite(rx, yy, SPR_FLAME, 11);
    }
  }
}

// Esfera con chispas que giran adentro.
void drawOrb(int cx, int cy, int r, uint32_t now, float speed, bool glow) {
  u8g2.drawCircle(cx, cy, r);
  if (glow) u8g2.drawCircle(cx, cy, r + 3);
  float a = now * speed / 1000.0f;
  for (int k = 0; k < 3; k++) {
    float ang = a + k * 2.094f;
    float rr = r * 0.55f * (0.7f + 0.3f * sinf(a * 1.7f + k));
    int size = 1 + ((now / 120 + k) % 3);
    drawSparkle(cx + (int)(cosf(ang) * rr), cy + (int)(sinf(ang) * rr * 0.8f), size);
  }
  // brillo
  int hx = cx - r / 2, hy = cy - r / 2;
  u8g2.drawPixel(hx, hy);
  u8g2.drawPixel(hx + 1, hy);
  u8g2.drawPixel(hx, hy + 1);
}

void drawPedestal(int cx, int top) {
  u8g2.drawBox(cx - 9, top, 18, 2);
  u8g2.drawLine(cx - 5, top + 2, cx - 5, top + 5);
  u8g2.drawLine(cx + 5, top + 2, cx + 5, top + 5);
  u8g2.drawBox(cx - 12, top + 6, 24, 2);
}

}  // namespace

void oracleEnter(uint32_t now) {
  ost = O_IDLE;
  ostAt = now;
  lastAct = now;
  u8g2.setContrast(255);
}

void oracleUpdate(uint32_t now) {
  bool anyBtn = btnL.shortRel || btnOk.shortRel || btnR.shortRel || comboFired;
  bool anyPress = btnL.press || btnOk.press || btnR.press;
  if (anyPress || shakeFired) lastAct = now;

  switch (ost) {
    case O_SLEEP:
      if (anyPress || shakeFired) {
        u8g2.setContrast(255);
        ost = O_IDLE;
        ostAt = now;
        if (shakeFired) ask(now);
      }
      break;

    case O_IDLE:
      if (shakeFired || anyBtn) ask(now);
      else if ((int32_t)(now - lastAct) > (int32_t)SLEEP_AFTER_MS) {
        ost = O_SLEEP;
        ostAt = now;
        u8g2.setContrast(1);
      }
      break;

    case O_SHAKE:
      if (now - ostAt > SHAKE_MS) {
        int i;
        do i = random(NPH); while (i == lastPhrase);
        lastPhrase = i;
        box.set(upperUtf8(PHRASES[i]), 124, 3);
        ost = O_REVEAL;
        ostAt = now;
        lastTyped = 0;
        music.play(&SONG_REVEAL);
      }
      break;

    case O_REVEAL:
      if (now - ostAt > TYPE_DELAY) {
        box.tick(now);
        if (box.typed != lastTyped) {
          if (box.typed > lastTyped && box.typed % 2 == 0) music.play(&SONG_BLIP);
          lastTyped = box.typed;
        }
      }
      if (anyBtn) {
        if (box.ok()) {
          ost = O_DONE;
          ostAt = now;
        }
      } else if (box.full() && box.lastPage()) {
        ost = O_DONE;
        ostAt = now;
      }
      break;

    case O_DONE:
      if (shakeFired || anyBtn) ask(now);
      else if (now - ostAt > DONE_TIMEOUT) {
        ost = O_IDLE;
        ostAt = now;
      }
      break;
  }
}

void oracleDraw(uint32_t now) {
  uint32_t st = now - ostAt;
  switch (ost) {
    case O_IDLE: {
      drawFlames(12, 107, 30, now, false);
      drawOrb(64, 24, 16, now, 1.2f, (now / 700) % 2);
      drawPedestal(64, 41);
      u8g2.setFont(u8g2_font_5x7_tf);
      centerText(63, TILT_ENABLED ? "AGITÁ Y PREGUNTÁ" : "PREGUNTÁ Y APRETÁ");
      break;
    }
    case O_SHAKE: {
      int amp = 1 + (int)(st * 3 / SHAKE_MS);
      int ox = random(-amp, amp + 1), oy = random(-amp, amp + 1);
      drawFlames(12, 107, 30, now, true);
      drawOrb(64 + ox, 24 + oy, 16, now, 3.0f + st * 0.01f, true);
      drawPedestal(64, 41);
      // chispas que se escapan
      for (int k = 0; k < 6; k++) {
        float ang = k * 1.047f + st * 0.004f;
        float rr = 18 + (st / 15 + k * 7) % 20;
        drawSparkle(64 + (int)(cosf(ang) * rr), 24 + (int)(sinf(ang) * rr * 0.6f), 1 + (k + st / 90) % 2);
      }
      u8g2.setFont(u8g2_font_5x7_tf);
      int dots = (st / 300) % 4;
      char buf[24] = "EL ORÁCULO PIENSA";
      for (int i = 0; i < dots; i++) strcat(buf, ".");
      u8g2.drawUTF8(20, 63, buf);
      break;
    }
    case O_REVEAL:
    case O_DONE: {
      box.draw(-1, 9, 10, now);
      drawFlames(22, 97, 35, now, false);
      int pct = ost == O_REVEAL && st < 700 ? (int)(st * 100 / 700) : 100;
      if (pct >= 100) drawSprite(58, 34, SPR_SAGE, 16);
      else drawSpriteDissolve(58, 34, SPR_SAGE, 16, pct);
      if (ost == O_DONE && (now / 600) % 2 == 0) {
        u8g2.setFont(u8g2_font_5x7_tf);
        centerText(63, TILT_ENABLED ? "AGITÁ DE NUEVO" : "APRETÁ DE NUEVO");
      }
      break;
    }
    case O_SLEEP: {
      // casi apagada; la esfera se mueve despacio para no marcar la pantalla
      int cx = 64 + (int)(36 * sinf(now / 9000.0f));
      int cy = 32 + (int)(14 * sinf(now / 7000.0f));
      drawOrb(cx, cy, 7, now, 0.6f, false);
      break;
    }
  }
}
