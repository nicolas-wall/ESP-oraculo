#pragma once

// =====================================================================
//  ORÁCULO
// =====================================================================

// Acelerómetro MPU-6050 (mismo I2C que la pantalla). Si no lo encuentra,
// cualquier botón le hace la pregunta al oráculo.
// Una sacudida = SHAKE_PEAKS golpes de más de SHAKE_G (en g) dentro de SHAKE_WINDOW_MS.
// Más alto = hay que agitar más fuerte.
#define SHAKE_G          0.6f
#define SHAKE_PEAKS      3
#define SHAKE_WINDOW_MS  900
#define SHAKE_DEBUG      0     // 1 = muestra las lecturas del sensor arriba de la pantalla

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
