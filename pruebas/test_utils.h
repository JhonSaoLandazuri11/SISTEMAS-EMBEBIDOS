#pragma once
// Utilidades comunes para las pruebas de componentes.
// Formato de salida (para capturar como evidencia en el informe):
//   [PASS] T01.2 Humedad dentro de rango 0-100 %  -> 41.0
//   [FAIL] T01.3 ...
//   ===== RESULTADO T01_DHT22: 5/6 PASS =====
#include <Arduino.h>

static int _tPass = 0;
static int _tTotal = 0;

inline void registrar(const char* id, const char* descripcion, bool ok,
                      const String& detalle = "") {
    _tTotal++;
    if (ok) _tPass++;
    Serial.print(ok ? "[PASS] " : "[FAIL] ");
    Serial.print(id);
    Serial.print(" ");
    Serial.print(descripcion);
    if (detalle.length() > 0) {
        Serial.print("  -> ");
        Serial.print(detalle);
    }
    Serial.println();
}

inline void informar(const char* texto) {
    Serial.print("[INFO] ");
    Serial.println(texto);
}

inline void pasoManual(const char* instruccion) {
    Serial.print("[MANUAL] ");
    Serial.println(instruccion);
}

inline void bannerPrueba(const char* nombre, const char* objetivo) {
    Serial.println();
    Serial.println("=====================================================");
    Serial.print("PRUEBA ");
    Serial.println(nombre);
    Serial.println(objetivo);
    Serial.println("=====================================================");
}

inline void resumenPrueba(const char* nombre) {
    Serial.println("-----------------------------------------------------");
    Serial.print("===== RESULTADO ");
    Serial.print(nombre);
    Serial.print(": ");
    Serial.print(_tPass);
    Serial.print("/");
    Serial.print(_tTotal);
    Serial.println(_tPass == _tTotal ? " PASS (TODO OK) =====" : " PASS (HAY FALLOS) =====");
}

// Espera hasta que cond() sea verdadera o venza el tiempo (ms).
template <typename F>
inline bool esperarHasta(F cond, unsigned long timeoutMs) {
    unsigned long t0 = millis();
    while (millis() - t0 < timeoutMs) {
        if (cond()) return true;
        delay(50);
    }
    return cond();
}
