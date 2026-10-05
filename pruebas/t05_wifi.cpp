// T05 - Conectividad Wi-Fi y DNS
#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "test_utils.h"

bool conectar(unsigned long timeoutMs) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    return esperarHasta([]() { return WiFi.status() == WL_CONNECTED; }, timeoutMs);
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    bannerPrueba("T05_WIFI", "Verifica conexion Wi-Fi, IP, senal, DNS y reconexion");

    // ---- T05.1: conexion ----
    unsigned long t0 = millis();
    bool conectado = conectar(20000);
    registrar("T05.1", "Conecta a la red Wi-Fi en <= 20 s", conectado,
              String(ssid) + " en " + String(millis() - t0) + " ms");
    if (!conectado) {
        resumenPrueba("T05_WIFI");
        return;
    }

    // ---- T05.2: IP valida ----
    IPAddress ip = WiFi.localIP();
    registrar("T05.2", "Se obtiene una IP distinta de 0.0.0.0 (DHCP)", ip != IPAddress(0, 0, 0, 0),
              ip.toString());

    // ---- T05.3: senal ----
    int rssi = WiFi.RSSI();
    registrar("T05.3", "RSSI valido (entre -100 y 0 dBm)", rssi < 0 && rssi > -100,
              String(rssi) + " dBm");

    // ---- T05.4 / T05.5: resolucion DNS ----
    IPAddress ipHive;
    bool dnsHive = WiFi.hostByName(mqtt_server, ipHive);
    registrar("T05.4", "DNS resuelve el broker HiveMQ Cloud", dnsHive, ipHive.toString());

    IPAddress ipTs;
    bool dnsTs = WiFi.hostByName("api.thingspeak.com", ipTs);
    registrar("T05.5", "DNS resuelve api.thingspeak.com", dnsTs, ipTs.toString());

    // ---- T05.6: reconexion tras perder el enlace ----
    WiFi.disconnect();
    delay(500);
    bool caido = WiFi.status() != WL_CONNECTED;
    bool volvio = conectar(20000);
    registrar("T05.6", "Tras desconectar, se reconecta en <= 20 s", caido && volvio);

    resumenPrueba("T05_WIFI");
}

void loop() { delay(1000); }
