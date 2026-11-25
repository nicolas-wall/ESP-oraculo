#pragma once

// =====================================================================
//  ORÁCULO
// =====================================================================

// Sensor de inclinación / sacudida: 1 = conectado, 0 = todavía no.
// Sin sensor, cualquier botón le hace la pregunta al oráculo.
#define TILT_ENABLED    0
#define PIN_TILT        D1     // la otra pata va a GND
// Cuántos cambios del sensor en menos de TILT_WINDOW_MS cuentan como "sacudida"
#define TILT_EDGES      4
#define TILT_WINDOW_MS  800

// Sin tocar nada por este tiempo, la pantalla se apaga casi del todo (ms)
#define SLEEP_AFTER_MS  120000

// =====================================================================
//  HARDWARE
// =====================================================================

// Tipo de OLED: 1 = SSD1309 (1.54"), 2 = SSD1306 (0.96"), 3 = SH1106 (1.3")
#define OLED_TYPE       1

// Pines (nombres de la XIAO ESP32-S3)
#define PIN_OLED_SDA    D4
#define PIN_OLED_SCL    D5
#define PIN_OLED_RES    U8X8_PIN_NONE
#define PIN_BUZZER      D3
#define PIN_BTN_PREV    D0   // botón izquierdo
#define PIN_BTN_OK      D8   // botón del medio
#define PIN_BTN_NEXT    D2   // botón derecho

// 1 = la otra pata de los botones va a GND, 0 = va a 3V3
#define BTN_TO_GND      1
