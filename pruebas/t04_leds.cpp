// T04 - LEDs de estado  |  verde=GPIO25  naranja=GPIO26  rojo=GPIO27
#include <Arduino.h>
#include "pines.h"
#include "logica.h"
#include "test_utils.h"

static const int PINES[3] = {LED_VERDE, LED_NARANJA, LED_ROJO};
static const char* NOMBRES[3] = {"VERDE", "NARANJA", "ROJO"};

void apagarTodos() {
    for (int i = 0; i < 3; i++) digitalWrite(PINES[i], LOW);
}

// Misma logica que controlarLEDs() de src/main.cpp
void aplicarEstado(EstadoAmbiental e) {
    apagarTodos();
    if (e == ESTADO_NORMAL) digitalWrite(LED_VERDE, HIGH);
    else if (e == ESTADO_ADVERTENCIA) digitalWrite(LED_NARANJA, HIGH);
    else digitalWrite(LED_ROJO, HIGH);
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    bannerPrueba("T04_LEDS", "Verifica cada LED y la logica de semaforo por temperatura");

    for (int i = 0; i < 3; i++) pinMode(PINES[i], OUTPUT);
    apagarTodos();

    // ---- T04.1 a T04.3: cada LED enciende y apaga (lectura del pin) ----
    for (int i = 0; i < 3; i++) {
        digitalWrite(PINES[i], HIGH);
        delay(600);
        bool alto = digitalRead(PINES[i]) == HIGH;
        digitalWrite(PINES[i], LOW);
        delay(300);
        bool bajo = digitalRead(PINES[i]) == LOW;
        char id[8];
        snprintf(id, sizeof(id), "T04.%d", i + 1);
        String desc = String("LED ") + NOMBRES[i] + " (GPIO" + PINES[i] + ") enciende y apaga";
        registrar(id, desc.c_str(), alto && bajo);
    }

    // ---- T04.4: solo un LED encendido a la vez, segun temperatura ----
    struct Caso { float t; int esperado; };  // indice del LED esperado
    const Caso casos[] = {
        {20.0f, 0}, {24.9f, 0},            // NORMAL    -> verde
        {25.0f, 1}, {27.5f, 1}, {30.0f, 1},  // ADVERTENCIA -> naranja
        {30.1f, 2}, {35.0f, 2}             // ALERTA    -> rojo
    };
    int ok = 0;
    const int n = sizeof(casos) / sizeof(casos[0]);
    for (int c = 0; c < n; c++) {
        aplicarEstado(clasificarTemperatura(casos[c].t));
        delay(700);
        bool correcto = true;
        for (int i = 0; i < 3; i++) {
            bool debeEstarEncendido = (i == casos[c].esperado);
            if ((digitalRead(PINES[i]) == HIGH) != debeEstarEncendido) correcto = false;
        }
        Serial.printf("   T=%.1f C -> LED %s %s\n", casos[c].t, NOMBRES[casos[c].esperado],
                      correcto ? "OK" : "ERROR");
        if (correcto) ok++;
    }
    registrar("T04.4", "Semaforo: 7 temperaturas encienden solo el LED correcto", ok == n,
              String(ok) + "/" + String(n));

    // ---- T04.5: todos apagados al terminar ----
    apagarTodos();
    delay(200);
    bool apagados = true;
    for (int i = 0; i < 3; i++) if (digitalRead(PINES[i]) != LOW) apagados = false;
    registrar("T04.5", "Los 3 LEDs quedan apagados al finalizar", apagados);

    pasoManual("Verifica visualmente que durante la prueba se vio: verde, naranja y rojo "
               "uno a la vez (captura de pantalla de cada estado como evidencia).");

    resumenPrueba("T04_LEDS");
}

void loop() { delay(1000); }
