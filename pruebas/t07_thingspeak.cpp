// T07 - Envio HTTP a ThingSpeak
// AVISO: esta prueba escribe 3 puntos de PRUEBA en tu canal (24.5/40, 26/42, ...).
// Usa un canal de pruebas o borra los datos despues (Channel Settings > Clear Channel).
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "config.h"
#include "logica.h"
#include "test_utils.h"

// Devuelve el codigo HTTP; 'cuerpo' recibe la respuesta (numero de entrada)
int enviar(const char* apiKey, float t, float h, int adc, String& cuerpo) {
    char url[256];
    if (construirUrlThingSpeak(url, sizeof(url), thingspeak_server, apiKey, t, h, adc) < 0)
        return -999;
    HTTPClient http;
    http.begin(url);
    int code = http.GET();
    cuerpo = (code > 0) ? http.getString() : http.errorToString(code);
    http.end();
    return code;
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    bannerPrueba("T07_THINGSPEAK", "Verifica envio de datos, respuesta, limite de tiempo y clave invalida");

    // ---- T07.0: precondiciones ----
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    bool wifi = esperarHasta([]() { return WiFi.status() == WL_CONNECTED; }, 20000);
    registrar("T07.0", "Precondicion: Wi-Fi conectado", wifi);
    bool claveConfigurada = String(thingspeak_api_key) != "TU_WRITE_API_KEY" &&
                            String(thingspeak_api_key).length() == 16;
    registrar("T07.1", "La Write API Key esta configurada (16 caracteres) en secrets.h",
              claveConfigurada, claveConfigurada ? "OK" : "edita include/secrets.h");
    registrar("T07.2", "Intervalo configurado >= 15 s (limite de la cuenta gratuita)",
              thingspeakInterval >= 15000, String(thingspeakInterval) + " ms");
    if (!wifi || !claveConfigurada) {
        resumenPrueba("T07_THINGSPEAK");
        return;
    }

    // ---- T07.3: envio valido ----
    String cuerpo;
    int code = enviar(thingspeak_api_key, 24.5f, 40.0f, 1213, cuerpo);
    long entrada = cuerpo.toInt();
    registrar("T07.3", "HTTP 200 al enviar T/H/ADC", code == 200, "HTTP " + String(code));
    registrar("T07.4", "La respuesta es un numero de entrada > 0 (0 = dato rechazado)", entrada > 0,
              "entrada #" + cuerpo);

    // ---- T07.5: limite de 15 s: un segundo envio inmediato debe ser rechazado ----
    String cuerpo2;
    int code2 = enviar(thingspeak_api_key, 25.0f, 41.0f, 1300, cuerpo2);
    registrar("T07.5", "Un 2.o envio inmediato (<15 s) es rechazado por ThingSpeak (respuesta 0)",
              code2 == 200 && cuerpo2.toInt() == 0, "HTTP " + String(code2) + ", respuesta=" + cuerpo2);

    // ---- T07.6: tras esperar el intervalo vuelve a aceptar ----
    informar("Esperando 20 s para el siguiente envio...");
    delay(thingspeakInterval);
    String cuerpo3;
    int code3 = enviar(thingspeak_api_key, 26.0f, 42.0f, 1400, cuerpo3);
    registrar("T07.6", "Tras 20 s, ThingSpeak acepta de nuevo el dato", code3 == 200 && cuerpo3.toInt() > 0,
              "entrada #" + cuerpo3);

    // ---- T07.7: clave invalida ----
    delay(thingspeakInterval);
    String cuerpo4;
    int code4 = enviar("CLAVEINVALIDA0000", 24.0f, 40.0f, 1000, cuerpo4);
    registrar("T07.7", "Con API Key invalida el dato NO se registra (entrada 0 o HTTP 4xx)",
              code4 > 0 && (cuerpo4.toInt() == 0 || code4 >= 400), "HTTP " + String(code4) + ", respuesta=" + cuerpo4);

    pasoManual("Abre tu canal de ThingSpeak (Private View) y confirma que aparecen los puntos "
               "24.5/40, 26.0/42 (captura como evidencia). Field1=Temp, Field2=Hum, Field3=ADC.");

    resumenPrueba("T07_THINGSPEAK");
}

void loop() { delay(1000); }
