# Oráculo y Mazmorra

Un aparatito de escritorio con estética de aventura clásica. Usa una **XIAO ESP32-S3**, una **OLED de 128x64**, un **buzzer pasivo**, **3 botones** y un **sensor de inclinación**.

- **Oráculo:** es lo que muestra siempre. Una esfera brilla entre dos fuegos. Le hacés una pregunta en voz alta y lo agitás: la esfera tiembla, aparece el sabio y te da un consejo letra por letra. Hay respuestas tipo bola 8, profecías y consejos absurdos.
- **Mazmorra:** mantené apretado **OK** para entrar, y otra vez para volver al oráculo. Es un juego por turnos visto desde arriba: buscá el cofre con la llave, abrí la escalera y bajá al piso siguiente. Cada piso es distinto y cada vez más difícil.

## Controles

**Siempre**

| Acción | Botón |
|---|---|
| Cambiar entre oráculo y mazmorra | mantener **OK** |
| Silenciar o activar el sonido | mantener **izquierda** |

**Oráculo:** agitalo. Sin sensor, cualquier botón le hace la pregunta. Después de 2 minutos sin uso, la pantalla se apaga casi del todo.

**Mazmorra**

| Acción | Botón |
|---|---|
| Girar | **izquierda** / **derecha** |
| Avanzar, atacar o abrir lo que tenés enfrente | **OK** |
| Poner una bomba | **izquierda + derecha** juntos |
| Ver el mapa del piso | mantener **derecha** |

- Los enemigos se mueven solo cuando vos avanzás. Girar no gasta turno.
- Las esquinitas que titilan marcan el casillero al que vas a avanzar.
- **Blob:** lento. **Murciélago:** se mueve medio al azar. **Caballero:** te persigue, aguanta 2 golpes y saca un corazón entero.
- Las bombas explotan a los 3 turnos, rompen bloques y lastiman a todo lo que esté alrededor, incluido vos.
- En algunos pisos, el cofre del premio trae un corazón extra.
- El récord (piso más profundo y rupias) se guarda aunque lo desenchufes.

## Conexiones

| Componente | Pin del módulo | XIAO ESP32-S3 |
|---|---|---|
| OLED | GND / VCC | GND / 3V3 |
| OLED | SCL / SDA | **D5 / D4** |
| Buzzer | S / medio / − | **D3** / 3V3 / GND |
| Botón izquierdo | entre pin y GND | **D0** |
| Botón del medio (OK) | entre pin y GND | **D8** |
| Botón derecho | entre pin y GND | **D2** |
| Sensor de inclinación | entre pin y GND | **D1** (configurable) |

## Personalizar

En [`include/config.h`](include/config.h):

- **`TILT_ENABLED`:** ponelo en 1 cuando conectes el sensor. **`PIN_TILT`** es el pin.
- **`TILT_EDGES` / `TILT_WINDOW_MS`:** qué tan fuerte hay que agitarlo.
- **`SLEEP_AFTER_MS`:** cuánto espera antes de apagar la pantalla.
- **`OLED_TYPE`:** 1 = SSD1309, 2 = SSD1306, 3 = SH1106.

Las frases del oráculo están al principio de [`src/oracle.cpp`](src/oracle.cpp). Podés agregar las tuyas.

## Compilar y subir

```bash
pio run -e xiao_esp32s3 -t upload
```

Si no aparece el puerto, mantené apretado el botón **B** (BOOT) de la XIAO mientras la conectás.

---
Proyecto de fan, con personajes, dibujos y música propios.
