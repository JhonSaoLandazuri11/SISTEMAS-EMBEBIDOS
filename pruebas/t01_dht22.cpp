// T01 - Sensor DHT22 (temperatura y humedad)  |  pin: DHT_PIN (GPIO4)
#include <Arduino.h>
#include <DHT.h>
#include "pines.h"
#include "logica.h"
#include "test_utils.h"

DHT dht(DHT_PIN, DHT_TYPE);

void setup() {
    Serial.begin(115200);
    delay(1000);
    bannerPrueba("T01_DHT22", "Verifica lectura, rango y respuesta del DHT22");

    dht.begin();
    delay(2000);  // el DHT22 necesita >= 2 s entre lecturas

    // ---- T01.1 a T01.3: 5 lecturas consecutivas ----
    const int N = 5;
    float t[N], h[N];
    int validas = 0;
    int enRango = 0;
    for (int i = 0; i < N; i++) {
        t[i] = dht.readTemperature();
        h[i] = dht.readHumidity();
        Serial.printf("   lectura %d: T=%.1f C  H=%.1f %%\n", i + 1, t[i], h[i]);
        if (!isnan(t[i]) && !isnan(h[i])) validas++;
        if (lecturaValida(t[i], h[i])) enRango++;
        delay(2100);
    }
    registrar("T01.1", "El sensor responde (sin NaN) en 5/5 lecturas", validas == N,
              String(validas) + "/5");
    registrar("T01.2", "Temperatura -40..80 C y humedad 0..100 % en todas", enRango == N,
              String(enRango) + "/5");

    float tMax = t[0], tMin = t[0];
    for (int i = 1; i < N; i++) {
        if (t[i] > tMax) tMax = t[i];
        if (t[i] < tMin) tMin = t[i];
    }
    registrar("T01.3", "Estabilidad: variación de T < 2 C con el sensor quieto",
              (tMax - tMin) < 2.0f, String(tMax - tMin, 2) + " C");

    // ---- T01.4: la lectura sigue al cambio (paso MANUAL) ----
    float base = t[N - 1];
    pasoManual("Cambia la temperatura del DHT22 (Wokwi: clic en el sensor y mueve el control; "
               "hardware real: calienta el sensor con la mano). Tienes 30 s.");
    bool cambio = esperarHasta([&]() {
        float v = dht.readTemperature();
        delay(2000);
        return !isnan(v) && fabsf(v - base) >= 2.0f;
    }, 30000);
    registrar("T01.4", "La lectura cambia >= 2 C al variar la temperatura (manual)", cambio);

    // ---- T01.5: clasificación de la lectura real ----
    float actual = dht.readTemperature();
    registrar("T01.5", "La temperatura actual se clasifica en un estado válido",
              !isnan(actual), String(nombreEstado(clasificarTemperatura(actual))));

    resumenPrueba("T01_DHT22");
}

void loop() { delay(1000); }
