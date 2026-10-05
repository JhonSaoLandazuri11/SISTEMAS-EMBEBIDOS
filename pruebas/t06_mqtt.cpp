// T06 - MQTT sobre TLS contra HiveMQ Cloud (puerto 8883)
// Detecta, entre otros, los problemas vistos en el proyecto:
//   * certificado CA que no valida (state -2)
//   * credencial con permiso "Subscribe only" (conecta pero el broker cierra al publicar)
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "config.h"
#include "certificado.h"
#include "test_utils.h"

WiFiClientSecure tlsClient;
PubSubClient mqtt(tlsClient);

static const char* TOPIC_PRUEBA = "sistema/ambiente/prueba";
volatile bool recibido = false;
String ultimoMensaje;

void alRecibir(char* topic, byte* payload, unsigned int len) {
    ultimoMensaje = "";
    for (unsigned int i = 0; i < len; i++) ultimoMensaje += (char)payload[i];
    recibido = true;
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    bannerPrueba("T06_MQTT", "Verifica TLS, autenticacion, publicar, suscribir y mantener la sesion");

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    bool wifi = esperarHasta([]() { return WiFi.status() == WL_CONNECTED; }, 20000);
    registrar("T06.0", "Precondicion: Wi-Fi conectado", wifi);
    if (!wifi) {
        resumenPrueba("T06_MQTT");
        return;
    }

    // ---- T06.1: handshake TLS validando el certificado CA ----
    tlsClient.setCACert(root_ca);
    bool tlsOk = tlsClient.connect(mqtt_server, mqtt_port);
    char errBuf[100] = "";
    tlsClient.lastError(errBuf, sizeof(errBuf));
    registrar("T06.1", "Handshake TLS valida el certificado (setCACert)", tlsOk,
              tlsOk ? String("OK") : String("error: ") + errBuf);
    tlsClient.stop();

    if (!tlsOk) {
        informar("Diagnostico: reintentando SIN validar certificado para aislar la causa...");
        tlsClient.setInsecure();
        bool sinValidar = tlsClient.connect(mqtt_server, mqtt_port);
        registrar("T06.1b", "Handshake TLS sin validar (setInsecure) - solo diagnostico", sinValidar,
                  sinValidar ? "el servidor responde: el problema es el certificado CA / la hora"
                             : "el servidor no responde: revisa host, puerto o red");
        tlsClient.stop();
        if (!sinValidar) {
            resumenPrueba("T06_MQTT");
            return;
        }
        // continuar el resto de pruebas sin validar certificado
    }

    mqtt.setServer(mqtt_server, mqtt_port);
    mqtt.setCallback(alRecibir);
    mqtt.setKeepAlive(15);

    // ---- T06.2: autenticacion correcta ----
    String id = "ESP32-Prueba-" + String(random(0xffff), HEX);
    bool conecto = mqtt.connect(id.c_str(), mqtt_user, mqtt_password);
    registrar("T06.2", "Conecta con usuario/clave de HiveMQ", conecto,
              "state=" + String(mqtt.state()) + " (0 = OK, 4/5 = credenciales)");
    if (!conecto) {
        resumenPrueba("T06_MQTT");
        return;
    }

    // ---- T06.3: suscripcion ----
    bool sub = mqtt.subscribe(TOPIC_PRUEBA);
    registrar("T06.3", "Se suscribe al topic de prueba", sub, TOPIC_PRUEBA);

    // ---- T06.4: publicar ----
    const char* msg = "{\"prueba\":\"T06\",\"valor\":123}";
    bool pub = mqtt.publish(TOPIC_PRUEBA, msg);
    registrar("T06.4", "Publica un mensaje JSON", pub, msg);

    // ---- T06.5: ida y vuelta (publica + suscribe => permiso Publish and Subscribe) ----
    bool vuelta = esperarHasta([]() { mqtt.loop(); return recibido; }, 8000);
    registrar("T06.5", "Recibe su propio mensaje (ida y vuelta; exige permiso Publish and Subscribe)",
              vuelta && ultimoMensaje == msg,
              vuelta ? ultimoMensaje : String("no llego; credencial 'Subscribe only' o 'Publish only'?"));

    // ---- T06.6: la sesion sobrevive tras publicar ----
    delay(2000);
    mqtt.loop();
    registrar("T06.6", "La conexion sigue activa 2 s despues de publicar", mqtt.connected(),
              "state=" + String(mqtt.state()));

    // ---- T06.7: keep-alive (20 s > keepAlive de 15 s) ----
    bool viva = true;
    unsigned long t0 = millis();
    while (millis() - t0 < 20000) {
        mqtt.loop();
        if (!mqtt.connected()) { viva = false; break; }
        delay(100);
    }
    registrar("T06.7", "La sesion se mantiene 20 s (keep-alive 15 s)", viva);

    // ---- T06.8: publicar el payload real del sistema ----
    char payload[128];
    snprintf(payload, sizeof(payload),
             "{\"temperatura\":24.00,\"humedad\":40.00,\"adc\":1213}");
    bool pubReal = mqtt.connected() && mqtt.publish(mqtt_topic, payload);
    registrar("T06.8", "Publica el payload real en sistema/ambiente", pubReal, payload);

    mqtt.disconnect();

    // ---- T06.9: credenciales incorrectas deben ser rechazadas ----
    tlsClient.stop();
    bool malo = mqtt.connect("ESP32-Prueba-neg", mqtt_user, "clave-incorrecta-xyz");
    registrar("T06.9", "Con clave incorrecta el broker RECHAZA la conexion", !malo,
              "state=" + String(mqtt.state()) + " (esperado 4 o 5)");

    pasoManual("En el Web Client de HiveMQ suscribete a 'sistema/#' y confirma que viste los mensajes "
               "de T06.4 y T06.8 (captura como evidencia).");

    resumenPrueba("T06_MQTT");
}

void loop() { delay(1000); }
