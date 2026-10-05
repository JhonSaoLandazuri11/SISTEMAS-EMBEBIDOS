// T08 - Modo Deep Sleep (ahorro de energia) y despertar por temporizador
// Esta prueba reinicia el ESP32 a proposito: se ejecuta en 2 fases (arranque -> sueno -> despertar).
#include <Arduino.h>
#include "esp_sleep.h"
#include "test_utils.h"

RTC_DATA_ATTR int arranques = 0;          // sobrevive al deep sleep
RTC_DATA_ATTR int pasadasFase1 = 0;       // resultados de la fase 1 (se pierden al reiniciar)

static const uint64_t SUENO_US = 5ULL * 1000000ULL;  // 5 s para la prueba

void setup() {
    Serial.begin(115200);
    delay(1000);
    arranques++;

    esp_sleep_wakeup_cause_t causa = esp_sleep_get_wakeup_cause();

    if (arranques == 1) {
        bannerPrueba("T08_DEEPSLEEP", "Verifica entrada a deep sleep y despertar por timer (fase 1/2)");
        registrar("T08.1", "Primer arranque: causa de reinicio NO es deep sleep",
                  causa != ESP_SLEEP_WAKEUP_TIMER, "causa=" + String((int)causa));
        esp_err_t r = esp_sleep_enable_timer_wakeup(SUENO_US);
        registrar("T08.2", "Temporizador de despertar configurado a 5 s", r == ESP_OK);
        pasadasFase1 = _tPass;
        Serial.println("[INFO] Entrando en deep sleep 5 s... (el monitor puede cortarse)");
        Serial.flush();
        esp_deep_sleep_start();  // no retorna
    }

    // Fase 2: despues de despertar
    bannerPrueba("T08_DEEPSLEEP", "Despues del deep sleep (fase 2/2)");
    registrar("T08.3", "Despierta por TEMPORIZADOR", causa == ESP_SLEEP_WAKEUP_TIMER,
              "causa=" + String((int)causa) + " (4 = timer)");
    registrar("T08.4", "La variable RTC conserva su valor tras dormir (arranques = 2)",
              arranques == 2, "arranques=" + String(arranques));
    _tPass += pasadasFase1;  // sumar la fase 1
    _tTotal += 2;
    resumenPrueba("T08_DEEPSLEEP");
    informar("Fin. Para repetir la prueba reinicia la simulacion.");
}

void loop() { delay(1000); }
