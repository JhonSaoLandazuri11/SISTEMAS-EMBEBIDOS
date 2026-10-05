// T03 - Potenciometro / ADC  |  pin: POT_PIN (GPIO34, ADC1)
#include <Arduino.h>
#include "pines.h"
#include "logica.h"
#include "test_utils.h"

void setup() {
    Serial.begin(115200);
    delay(1000);
    bannerPrueba("T03_ADC", "Verifica lectura analogica, rango, estabilidad y recorrido");

    pinMode(POT_PIN, INPUT);

    // ---- T03.1: 20 muestras dentro de 0..4095 ----
    const int N = 20;
    int muestras[N];
    long suma = 0;
    int fuera = 0;
    int minV = 4095, maxV = 0;
    for (int i = 0; i < N; i++) {
        muestras[i] = analogRead(POT_PIN);
        suma += muestras[i];
        if (!adcValido(muestras[i])) fuera++;
        if (muestras[i] < minV) minV = muestras[i];
        if (muestras[i] > maxV) maxV = muestras[i];
        delay(20);
    }
    int media = suma / N;
    registrar("T03.1", "20 lecturas dentro del rango 0..4095", fuera == 0,
              "media=" + String(media));

    // ---- T03.2: estabilidad con el potenciometro quieto ----
    registrar("T03.2", "Ruido (max-min) < 100 cuentas con el potenciometro quieto",
              (maxV - minV) < 100, "max-min=" + String(maxV - minV));

    // ---- T03.3: conversion a voltaje coherente ----
    float v = adcAVoltaje(media);
    registrar("T03.3", "Voltaje calculado entre 0 y 3.3 V", v >= 0.0f && v <= 3.3f,
              String(v, 2) + " V");

    // ---- T03.4 / T03.5: recorrido completo (manual) ----
    pasoManual("Gira el potenciometro al MINIMO (0 V). Tienes 30 s.");
    bool llegoMin = esperarHasta([]() { return analogRead(POT_PIN) < 150; }, 30000);
    registrar("T03.4", "Al girar al minimo la lectura baja de 150 (manual)", llegoMin);

    pasoManual("Ahora gira el potenciometro al MAXIMO (3.3 V). Tienes 30 s.");
    bool llegoMax = esperarHasta([]() { return analogRead(POT_PIN) > 3900; }, 30000);
    registrar("T03.5", "Al girar al maximo la lectura supera 3900 (manual)", llegoMax);

    // ---- T03.6: monotonia en el barrido (informativo) ----
    informar("Mueve el potenciometro lentamente 10 s para ver la lectura cambiar:");
    unsigned long t0 = millis();
    while (millis() - t0 < 10000) {
        int a = analogRead(POT_PIN);
        Serial.printf("   ADC=%4d  (%.2f V)\n", a, adcAVoltaje(a));
        delay(500);
    }

    resumenPrueba("T03_ADC");
}

void loop() { delay(1000); }
