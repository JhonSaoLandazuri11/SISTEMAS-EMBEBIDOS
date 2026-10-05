// T02 - Pantalla OLED SSD1306 (I2C)  |  SDA=21 SCL=22 dir=0x3C
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "pines.h"
#include "logica.h"
#include "test_utils.h"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup() {
    Serial.begin(115200);
    delay(1000);
    bannerPrueba("T02_OLED", "Verifica bus I2C, inicializacion, buffer y dibujo del OLED");

    Wire.begin(OLED_SDA, OLED_SCL);

    // ---- T02.1: el dispositivo responde en 0x3C ----
    Wire.beginTransmission(OLED_ADDRESS);
    uint8_t err = Wire.endTransmission();
    registrar("T02.1", "El OLED responde en la direccion I2C 0x3C", err == 0,
              "codigo I2C=" + String(err));

    // ---- T02.2: inicializacion ----
    bool iniciado = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
    registrar("T02.2", "display.begin() inicializa el controlador SSD1306", iniciado);
    if (!iniciado) {
        resumenPrueba("T02_OLED");
        return;
    }

    // ---- T02.3: el buffer se puede limpiar y escribir ----
    display.clearDisplay();
    bool limpio = !display.getPixel(10, 10);
    display.drawPixel(10, 10, SSD1306_WHITE);
    bool encendido = display.getPixel(10, 10);
    registrar("T02.3", "Buffer: clearDisplay() apaga y drawPixel() enciende un pixel",
              limpio && encendido);

    // ---- T02.4: pantalla de datos (inspeccion visual) ----
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);   display.println("SISTEMA AMBIENTAL");
    display.setCursor(0, 16);  display.println("Temp: 24.0 C");
    display.setCursor(0, 28);  display.println("Hum:  40.0 %");
    display.setCursor(0, 40);  display.println("ADC:  1213");
    display.setCursor(0, 52);  display.println(textoEstadoOLED(ESTADO_NORMAL));
    display.display();
    pasoManual("Mira el OLED: deben verse 5 lineas (titulo, Temp, Hum, ADC, Estado). "
               "Toma una captura como evidencia. (5 s)");
    delay(5000);
    informar("Texto de datos mostrado (verificacion visual)");

    // ---- T02.5: los tres textos de estado caben y se dibujan ----
    EstadoAmbiental estados[3] = {ESTADO_NORMAL, ESTADO_ADVERTENCIA, ESTADO_ALERTA};
    bool todosCaben = true;
    for (int i = 0; i < 3; i++) {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println(textoEstadoOLED(estados[i]));
        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(textoEstadoOLED(estados[i]), 0, 0, &x1, &y1, &w, &h);
        if (w > SCREEN_WIDTH) todosCaben = false;
        display.display();
        delay(1200);
    }
    registrar("T02.5", "Los 3 textos de estado caben en 128 px de ancho", todosCaben);

    // ---- T02.6: patron de pixeles (todo blanco / borde) ----
    display.clearDisplay();
    display.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
    display.display();
    delay(1200);
    display.clearDisplay();
    display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
    display.display();
    pasoManual("Se mostro pantalla completa blanca y luego un borde rectangular: "
               "confirma que no hay pixeles muertos ni cortes.");
    registrar("T02.6", "Patron blanco total y borde ejecutados sin error", true);

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("T02 OLED: OK");
    display.display();

    resumenPrueba("T02_OLED");
}

void loop() { delay(1000); }
