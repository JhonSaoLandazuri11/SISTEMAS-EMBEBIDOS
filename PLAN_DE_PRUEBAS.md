# Plan de pruebas — Sistema Ambiental IoT (ESP32)

Sistema: ESP32 + DHT22 + OLED SSD1306 + potenciómetro (ADC) + 3 LEDs, con envío a **HiveMQ Cloud (MQTT/TLS)** y **ThingSpeak (HTTP)**.

Hay tres niveles de prueba. Cada prueba imprime líneas `[PASS]` / `[FAIL]` y un resumen `===== RESULTADO ... =====` que sirve como evidencia.

| Nivel | Qué prueba | Dónde corre | Comando |
|---|---|---|---|
| **A. Unitarias (lógica)** | Clasificación de estados, validación de lecturas, ADC→voltaje, JSON MQTT, URL de ThingSpeak, temporización | PC (sin hardware) | `pio test -e native` |
| **B. Del PC (Python)** | Simulador `simulador.py` y cableado `diagrama.json` vs. pines del firmware | PC | `python -m unittest discover -s pruebas/pc -v` |
| **C. Por componente** | DHT22, OLED, ADC, LEDs, Wi-Fi, MQTT, ThingSpeak, Deep Sleep | ESP32 real o Wokwi | `pio run -e t01_dht22` … (ver abajo) |
| **D. Integración** | Firmware completo extremo a extremo | ESP32 real o Wokwi | `pio run -e esp32dev` |

## 1. Preparación

1. Copia `include/secrets.example.h` como `include/secrets.h` y completa: usuario/clave de HiveMQ (permiso **Publish and Subscribe**) y la Write API Key de ThingSpeak. `secrets.h` está en `.gitignore`.
2. Para las pruebas T07 usa un **canal de ThingSpeak de pruebas** (escriben 3 puntos falsos). Si usas el canal real, bórralos después (*Channel Settings → Clear Channel*).
3. Para ejecutar una prueba de componente en Wokwi (extensión de VS Code), cambia las rutas de `wokwi.toml` al entorno que quieras probar y compila:
   ```toml
   firmware = ".pio/build/t01_dht22/firmware.bin"
   elf      = ".pio/build/t01_dht22/firmware.elf"
   ```
   En **Wokwi web**: crea un proyecto ESP32, pega el `.cpp` de la prueba como `sketch.ino` y añade como archivos `pines.h`, `logica.h`, `test_utils.h`, `config.h`, `secrets.h` y `certificado.h` (de `include/` y `pruebas/`). El `diagrama.json` es el mismo para todas las pruebas.
4. Para el PC necesitas PlatformIO (`pio`) y Python 3. El simulador necesita `pip install paho-mqtt` y las variables `MQTT_USER` y `MQTT_PASS`.

## 2. Pruebas por componente (nivel C)

Tipo: **A** = automática (el sketch decide PASS/FAIL) · **M** = requiere acción manual que se indica por el monitor serie.

### T01 — DHT22 (`pio run -e t01_dht22`)
| ID | Prueba | Criterio de aceptación | Tipo |
|---|---|---|---|
| T01.1 | Responde sin `NaN` | 5/5 lecturas válidas | A |
| T01.2 | Rango físico | −40…80 °C y 0…100 % en las 5 lecturas | A |
| T01.3 | Estabilidad | Variación de T < 2 °C con el sensor quieto | A |
| T01.4 | Sigue los cambios | La T cambia ≥ 2 °C al mover el sensor (30 s) | M |
| T01.5 | Clasificación | La T actual se traduce a NORMAL/ADVERTENCIA/ALERTA | A |

### T02 — OLED SSD1306 (`pio run -e t02_oled`)
| ID | Prueba | Criterio | Tipo |
|---|---|---|---|
| T02.1 | Bus I2C | Dispositivo responde en 0x3C | A |
| T02.2 | Inicialización | `display.begin()` devuelve true | A |
| T02.3 | Buffer | `clearDisplay` apaga y `drawPixel` enciende un píxel | A |
| T02.4 | Pantalla de datos | Se ven 5 líneas (título, Temp, Hum, ADC, Estado) | M (captura) |
| T02.5 | Textos de estado | Los 3 textos caben en 128 px | A |
| T02.6 | Patrón de píxeles | Pantalla blanca y borde sin cortes ni píxeles muertos | M |

### T03 — Potenciómetro / ADC (`pio run -e t03_adc`)
| ID | Prueba | Criterio | Tipo |
|---|---|---|---|
| T03.1 | Rango | 20 lecturas dentro de 0…4095 | A |
| T03.2 | Ruido | max − min < 100 cuentas con el potenciómetro quieto | A |
| T03.3 | Voltaje | 0…3,3 V coherente con el ADC | A |
| T03.4 | Mínimo | Lectura < 150 al girar al mínimo | M |
| T03.5 | Máximo | Lectura > 3900 al girar al máximo | M |

### T04 — LEDs (`pio run -e t04_leds`)
| ID | Prueba | Criterio | Tipo |
|---|---|---|---|
| T04.1–3 | LED verde / naranja / rojo | Cada pin pasa a HIGH y vuelve a LOW | A (+ vista) |
| T04.4 | Semáforo | 7 temperaturas (20, 24,9, 25, 27,5, 30, 30,1, 35 °C) encienden **solo** el LED correcto | A |
| T04.5 | Apagado final | Los 3 LEDs en LOW | A |

### T05 — Wi-Fi (`pio run -e t05_wifi`)
| ID | Prueba | Criterio | Tipo |
|---|---|---|---|
| T05.1 | Conexión | Conecta en ≤ 20 s | A |
| T05.2 | DHCP | IP distinta de 0.0.0.0 | A |
| T05.3 | Señal | RSSI entre −100 y 0 dBm | A |
| T05.4 | DNS HiveMQ | Resuelve el host del broker | A |
| T05.5 | DNS ThingSpeak | Resuelve `api.thingspeak.com` | A |
| T05.6 | Reconexión | Tras `disconnect()` vuelve en ≤ 20 s | A |

### T06 — MQTT / TLS / HiveMQ (`pio run -e t06_mqtt`)
| ID | Prueba | Criterio | Tipo |
|---|---|---|---|
| T06.0 | Precondición | Wi-Fi conectado | A |
| T06.1 | TLS con CA | Handshake válido con `setCACert(root_ca)` (si falla, T06.1b aísla si es el certificado o la red) | A |
| T06.2 | Autenticación | `connect()` con usuario/clave → state 0 | A |
| T06.3 | Suscripción | `subscribe()` correcto | A |
| T06.4 | Publicación | `publish()` correcto | A |
| T06.5 | Ida y vuelta | Recibe su propio mensaje (valida permiso **Publish and Subscribe**) | A |
| T06.6 | Sesión estable | Sigue conectado 2 s tras publicar (detecta credencial *Subscribe only*) | A |
| T06.7 | Keep-alive | Sesión viva 20 s con keep-alive de 15 s | A |
| T06.8 | Payload real | Publica en `sistema/ambiente` | A |
| T06.9 | Clave incorrecta | El broker **rechaza** la conexión (state 4/5) | A |
| — | Evidencia | Mensajes visibles en el *Web Client* de HiveMQ (`sistema/#`) | M (captura) |

### T07 — ThingSpeak (`pio run -e t07_thingspeak`)
| ID | Prueba | Criterio | Tipo |
|---|---|---|---|
| T07.0 | Precondición | Wi-Fi conectado | A |
| T07.1 | API Key | Configurada (16 caracteres) en `secrets.h` | A |
| T07.2 | Intervalo | `thingspeakInterval` ≥ 15 000 ms | A |
| T07.3 | Envío válido | HTTP 200 | A |
| T07.4 | Registro | Respuesta = número de entrada > 0 | A |
| T07.5 | Límite 15 s | Un 2.º envío inmediato es rechazado (respuesta 0) | A |
| T07.6 | Recuperación | Tras 20 s vuelve a aceptar | A |
| T07.7 | Clave inválida | El dato no se registra | A |
| — | Evidencia | Puntos visibles en el canal (Field1 Temp, Field2 Hum, Field3 ADC) | M (captura) |

### T08 — Deep Sleep (`pio run -e t08_deepsleep`)
La prueba se reinicia sola a propósito (fase 1 → dormir 5 s → fase 2).
| ID | Prueba | Criterio | Tipo |
|---|---|---|---|
| T08.1 | Primer arranque | Causa de reinicio ≠ timer | A |
| T08.2 | Temporizador | `esp_sleep_enable_timer_wakeup` → `ESP_OK` | A |
| T08.3 | Despertar | Causa = `ESP_SLEEP_WAKEUP_TIMER` | A |
| T08.4 | Memoria RTC | La variable `RTC_DATA_ATTR` conserva su valor | A |

## 3. Integración (nivel D) — firmware completo (`pio run -e esp32dev`)

| ID | Escenario | Resultado esperado | Evidencia |
|---|---|---|---|
| I1 | Arranque | Serial muestra DHT22/OLED/Wi-Fi/TLS/MQTT inicializados sin errores | captura del monitor |
| I2 | T < 25 °C | LED verde, OLED “Estado: NORMAL”, serial “NORMAL” | foto de la simulación |
| I3 | 25 ≤ T ≤ 30 °C | LED naranja, OLED “ALERTA MEDIA”, serial “ADVERTENCIA” | foto |
| I4 | T > 30 °C | LED rojo, OLED “PELIGRO”, serial “ALERTA” | foto |
| I5 | MQTT | Un JSON nuevo en `sistema/ambiente` cada ~5 s | Web Client de HiveMQ |
| I6 | ThingSpeak | Un punto nuevo cada ≥ 20 s | gráfica del canal |
| I7 | Coherencia | Mismo valor de T/H/ADC en OLED, serial, MQTT y ThingSpeak | las 4 capturas juntas |
| I8 | Fallo de lectura | Con DHT22 desconectado: “Error al leer el DHT22” y no se publica basura | captura del monitor |
| I9 | Pérdida de red | Al cortar Wi-Fi/MQTT el sistema se recupera sin reiniciar manualmente | captura del monitor |

## 4. Pruebas del PC (niveles A y B)

**Unitarias (`test/test_logica`, 21 casos)** — estados en los límites exactos (24,9 / 25 / 30 / 30,1), `NaN`, textos del OLED (caben en 21 caracteres), validación del DHT22, rango y voltaje del ADC, JSON de MQTT (formato, valores extremos, buffer pequeño), URL de ThingSpeak (2 y 3 campos), temporización con desbordamiento de `millis()`.

**Simulador (`pruebas/pc/test_simulador.py`, S1–S3)** — formato y rangos de las lecturas, JSON, que las credenciales no estén en el código, salida limpia sin variables de entorno, publicación con TLS y manejo de error de publicación (con un broker falso).

**Cableado (`pruebas/pc/test_cableado.py`, W0–W13)** — cada periférico del diagrama está en el GPIO que usa el firmware (`include/pines.h`), alimentación y tierra, dirección I2C, resistencias de 220 Ω en serie con cada LED, potenciómetro en un pin ADC1, pines que existen en cada componente de Wokwi, sin conexiones duplicadas ni GPIO repetidos.

## 5. Registro de resultados (rellenar)

| Prueba | Fecha | Resultado (n/N) | Archivo de evidencia |
|---|---|---|---|
| Unitarias `pio test -e native` | | | |
| Python `unittest` | | | |
| T01 DHT22 | | | |
| T02 OLED | | | |
| T03 ADC | | | |
| T04 LEDs | | | |
| T05 Wi-Fi | | | |
| T06 MQTT | | | |
| T07 ThingSpeak | | | |
| T08 Deep Sleep | | | |
| Integración I1–I9 | | | |

## 6. Hallazgos al revisar el proyecto

1. **El `main.cpp` del proyecto no es la versión que funciona en Wokwi.** Usa `reconnectMQTT()` bloqueante (un `delay(5000)` dentro de un `while`), valida TLS con `setCACert`, se suscribe al mismo topic donde publica, y solo envía 2 campos a ThingSpeak (la versión de Wokwi enviaba 3, con el ADC en `field3`). **T06.1** dirá si el certificado valida el servidor.
2. **`entrarDeepSleep()` nunca se llama** (código sin usar). T08 prueba el mecanismo, pero el firmware no lo usa.
3. **`simulador.py` no podía funcionar:** pasaba el usuario y la clave de HiveMQ como *nombres* de variable a `os.getenv(...)`, en lugar de nombres como `MQTT_USER`. Siempre terminaba con “Configura las variables…”. Ahora lee `MQTT_USER` y `MQTT_PASS`.
4. **El simulador nunca llega a ALERTA:** la temperatura máxima es 24 + 5,5 = 29,5 °C, así que la rama `> 30` (ADC 1500–3000) es código muerto. Está marcado como `expectedFailure` en `test_s1_6`.
5. **`diagrama.json`:** tenía 5 conexiones con pines inexistentes (`esp:D21`, `D22`, `D34`, `D25` y `led1:K`), cada una duplicada por otra válida. Se eliminaron. Además el archivo se llama `diagrama.json`; la extensión de Wokwi para VS Code busca `diagram.json`.
6. **Credenciales:** estaban escritas en `main.cpp` y `simulador.py`. Ahora están en `include/secrets.h` (ignorado por Git) y en variables de entorno. Como estuvieron en texto plano, **cambia la clave de HiveMQ** si el proyecto ya se compartió.
