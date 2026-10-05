#pragma once
// =====================================================
// LÓGICA PURA DEL SISTEMA (sin dependencias de Arduino)
// Se usa en src/main.cpp y se prueba en el PC con:
//     pio test -e native
// =====================================================
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

// ---------- Estados ambientales ----------
enum EstadoAmbiental {
    ESTADO_NORMAL = 0,
    ESTADO_ADVERTENCIA = 1,
    ESTADO_ALERTA = 2
};

static const float UMBRAL_ADVERTENCIA = 25.0f;  // °C
static const float UMBRAL_ALERTA = 30.0f;       // °C

// < 25 NORMAL | 25..30 (inclusive) ADVERTENCIA | > 30 ALERTA
// Un valor no numérico se trata como ALERTA (falla segura).
inline EstadoAmbiental clasificarTemperatura(float t) {
    if (isnan(t)) return ESTADO_ALERTA;
    if (t < UMBRAL_ADVERTENCIA) return ESTADO_NORMAL;
    if (t <= UMBRAL_ALERTA) return ESTADO_ADVERTENCIA;
    return ESTADO_ALERTA;
}

inline const char* nombreEstado(EstadoAmbiental e) {
    switch (e) {
        case ESTADO_NORMAL:      return "NORMAL";
        case ESTADO_ADVERTENCIA: return "ADVERTENCIA";
        default:                 return "ALERTA";
    }
}

inline const char* textoEstadoOLED(EstadoAmbiental e) {
    switch (e) {
        case ESTADO_NORMAL:      return "Estado: NORMAL";
        case ESTADO_ADVERTENCIA: return "Estado: ALERTA MEDIA";
        default:                 return "Estado: PELIGRO";
    }
}

// ---------- Validación de sensores ----------
// Rango de operación del DHT22: -40..80 °C, 0..100 %HR
inline bool lecturaValida(float t, float h) {
    if (isnan(t) || isnan(h)) return false;
    if (t < -40.0f || t > 80.0f) return false;
    if (h < 0.0f || h > 100.0f) return false;
    return true;
}

// ADC del ESP32: 12 bits
static const int ADC_MAX = 4095;

inline bool adcValido(int adc) { return adc >= 0 && adc <= ADC_MAX; }

inline float adcAVoltaje(int adc, float vref = 3.3f) {
    return (float)adc * vref / (float)ADC_MAX;
}

// ---------- Formato de mensajes ----------
// Devuelve el número de caracteres escritos, o -1 si no cabe en el buffer.
inline int construirPayload(char* buf, size_t n, float t, float h, int adc) {
    int len = snprintf(buf, n,
        "{\"temperatura\":%.2f,\"humedad\":%.2f,\"adc\":%d}", t, h, adc);
    return (len < 0 || (size_t)len >= n) ? -1 : len;
}

// adc < 0 => no se incluye field3
inline int construirUrlThingSpeak(char* buf, size_t n, const char* base,
                                  const char* apiKey, float t, float h,
                                  int adc = -1) {
    int len;
    if (adc >= 0) {
        len = snprintf(buf, n, "%s?api_key=%s&field1=%.2f&field2=%.2f&field3=%d",
                       base, apiKey, t, h, adc);
    } else {
        len = snprintf(buf, n, "%s?api_key=%s&field1=%.2f&field2=%.2f",
                       base, apiKey, t, h);
    }
    return (len < 0 || (size_t)len >= n) ? -1 : len;
}

// ---------- Temporización (segura ante desbordamiento de millis) ----------
inline bool intervaloCumplido(uint32_t ahora, uint32_t ultimo, uint32_t intervalo) {
    return (uint32_t)(ahora - ultimo) >= intervalo;
}
