#pragma once
// Configuración NO secreta del sistema. Las credenciales están en secrets.h
#include "secrets.h"

// ---------- Wi-Fi ----------
// Simulación Wokwi: "Wokwi-GUEST" sin clave. Hardware real: tu red.
static const char* ssid     = "Wokwi-GUEST";
static const char* password = "";

// ---------- HiveMQ Cloud ----------
static const char* mqtt_server =
    "72132f2da69e492e9e8cef84550f5c24.s1.eu.hivemq.cloud";
static const int   mqtt_port  = 8883;
static const char* mqtt_topic = "sistema/ambiente";

// ---------- ThingSpeak ----------
static const char* thingspeak_server = "http://api.thingspeak.com/update";
// ThingSpeak (cuenta gratuita) exige >= 15 s entre actualizaciones.
static const unsigned long thingspeakInterval = 20000;

// ---------- Deep Sleep ----------
// 1 = cada ciclo: leer -> publicar -> dormir. 0 = funcionamiento continuo.
#define USAR_DEEP_SLEEP 1
#define DEEP_SLEEP_SEGUNDOS 30